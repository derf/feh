/* gif.c

Animated GIF support for feh using giflib.
Decodes all frames into pre-composited Imlib_Image objects and
cycles through them using feh's timer system.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies of the Software and its documentation and acknowledgment shall be
given in the documentation and software packages that this Software was
used.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifdef HAVE_LIBGIF

#include "feh.h"
#include "gif.h"
#include "filelist.h"
#include "winwidget.h"
#include "timers.h"
#include "options.h"

#include <gif_lib.h>
#include <string.h>
#include <strings.h>

#define GIF_TIMER_NAME "GIF_FRAME"
#define GIF_DEFAULT_DELAY 10  /* centiseconds (100ms) */
#define GIF_MIN_DELAY 2       /* treat delays < 2cs as default */

static void cb_gif_frame_timer(void *data);

gif_anim *gif_load(const char *filename)
{
	int error = 0;
	GifFileType *gif = DGifOpenFileName(filename, &error);
	if (!gif)
		return NULL;

	if (DGifSlurp(gif) != GIF_OK) {
		DGifCloseFile(gif, &error);
		return NULL;
	}

	if (gif->ImageCount <= 1) {
		DGifCloseFile(gif, &error);
		return NULL;
	}

	int sw = gif->SWidth;
	int sh = gif->SHeight;

	if (sw <= 0 || sh <= 0) {
		DGifCloseFile(gif, &error);
		return NULL;
	}

	gif_anim *anim = calloc(1, sizeof(gif_anim));
	if (!anim) {
		DGifCloseFile(gif, &error);
		return NULL;
	}

	anim->frame_count = gif->ImageCount;
	anim->frames = calloc(anim->frame_count, sizeof(Imlib_Image));
	anim->delays = calloc(anim->frame_count, sizeof(int));
	anim->current_frame = 0;
	anim->loop_count = 0;
	anim->loops_done = 0;

	if (!anim->frames || !anim->delays) {
		gif_free(anim);
		DGifCloseFile(gif, &error);
		return NULL;
	}

	/* Check for NETSCAPE2.0 application extension (loop count) */
	for (int i = 0; i < gif->ImageCount; i++) {
		SavedImage *si = &gif->SavedImages[i];
		for (int j = 0; j < si->ExtensionBlockCount; j++) {
			ExtensionBlock *eb = &si->ExtensionBlocks[j];
			if (eb->Function == APPLICATION_EXT_FUNC_CODE
					&& eb->ByteCount >= 11
					&& memcmp(eb->Bytes, "NETSCAPE2.0", 11) == 0) {
				if (j + 1 < si->ExtensionBlockCount) {
					ExtensionBlock *sub = &si->ExtensionBlocks[j + 1];
					if (sub->ByteCount >= 3 && sub->Bytes[0] == 1) {
						anim->loop_count = sub->Bytes[1] | (sub->Bytes[2] << 8);
					}
				}
			}
		}
	}

	/* Compositing canvas */
	DATA32 *canvas = calloc(sw * sh, sizeof(DATA32));
	DATA32 *prev_canvas = calloc(sw * sh, sizeof(DATA32));

	if (!canvas || !prev_canvas) {
		free(canvas);
		free(prev_canvas);
		gif_free(anim);
		DGifCloseFile(gif, &error);
		return NULL;
	}

	for (int i = 0; i < gif->ImageCount; i++) {
		SavedImage *frame = &gif->SavedImages[i];
		GifImageDesc *desc = &frame->ImageDesc;

		GraphicsControlBlock gcb;
		gcb.DelayTime = GIF_DEFAULT_DELAY;
		gcb.DisposalMode = DISPOSAL_UNSPECIFIED;
		gcb.TransparentColor = -1;
		DGifSavedExtensionToGCB(gif, i, &gcb);

		int delay = gcb.DelayTime;
		if (delay < GIF_MIN_DELAY)
			delay = GIF_DEFAULT_DELAY;
		anim->delays[i] = delay;

		/* Save canvas for DISPOSE_PREVIOUS */
		if (gcb.DisposalMode == DISPOSE_PREVIOUS)
			memcpy(prev_canvas, canvas, sw * sh * sizeof(DATA32));

		ColorMapObject *cmap = desc->ColorMap ? desc->ColorMap : gif->SColorMap;
		if (!cmap) {
			/* No colormap, create a blank frame */
			Imlib_Image img = imlib_create_image(sw, sh);
			if (img) {
				imlib_context_set_image(img);
				imlib_image_set_has_alpha(1);
				DATA32 *data = imlib_image_get_data();
				memcpy(data, canvas, sw * sh * sizeof(DATA32));
				imlib_image_put_back_data(data);
			}
			anim->frames[i] = img;
			continue;
		}

		/* Paint frame pixels onto canvas */
		int fx = desc->Left;
		int fy = desc->Top;
		int fw = desc->Width;
		int fh = desc->Height;

		for (int y = 0; y < fh; y++) {
			for (int x = 0; x < fw; x++) {
				int cx = fx + x;
				int cy = fy + y;
				if (cx >= sw || cy >= sh)
					continue;

				int idx = frame->RasterBits[y * fw + x];

				if (idx == gcb.TransparentColor)
					continue;

				if (idx >= cmap->ColorCount)
					continue;

				GifColorType *c = &cmap->Colors[idx];
				canvas[cy * sw + cx] = (0xFFu << 24)
					| ((DATA32)c->Red << 16)
					| ((DATA32)c->Green << 8)
					| (DATA32)c->Blue;
			}
		}

		/* Snapshot the composited canvas as this frame */
		Imlib_Image img = imlib_create_image(sw, sh);
		if (!img) {
			free(canvas);
			free(prev_canvas);
			gif_free(anim);
			DGifCloseFile(gif, &error);
			return NULL;
		}
		imlib_context_set_image(img);
		imlib_image_set_has_alpha(1);
		DATA32 *data = imlib_image_get_data();
		memcpy(data, canvas, sw * sh * sizeof(DATA32));
		imlib_image_put_back_data(data);
		anim->frames[i] = img;

		/* Apply disposal method for next frame */
		switch (gcb.DisposalMode) {
		case DISPOSE_BACKGROUND:
			for (int y = 0; y < fh; y++) {
				for (int x = 0; x < fw; x++) {
					int cx = fx + x;
					int cy = fy + y;
					if (cx < sw && cy < sh)
						canvas[cy * sw + cx] = 0;
				}
			}
			break;
		case DISPOSE_PREVIOUS:
			memcpy(canvas, prev_canvas, sw * sh * sizeof(DATA32));
			break;
		case DISPOSAL_UNSPECIFIED:
		case DISPOSE_DO_NOT:
		default:
			break;
		}
	}

	free(canvas);
	free(prev_canvas);
	DGifCloseFile(gif, &error);
	return anim;
}

void gif_free(gif_anim *anim)
{
	if (!anim)
		return;

	if (anim->frames) {
		for (int i = 0; i < anim->frame_count; i++) {
			if (anim->frames[i]) {
				imlib_context_set_image(anim->frames[i]);
				imlib_free_image_and_decache();
			}
		}
		free(anim->frames);
	}
	free(anim->delays);
	free(anim);
}

static int is_gif_file(const char *filename)
{
	const char *ext = strrchr(filename, '.');
	if (!ext)
		return 0;
	return (strcasecmp(ext, ".gif") == 0);
}

void gif_animation_start(winwidget w)
{
	if (!w || !w->file || !w->file->data)
		return;

	feh_file *file = FEH_FILE(w->file->data);
	if (!is_gif_file(file->filename))
		return;

	gif_anim *anim = gif_load(file->filename);
	if (!anim)
		return;

	/* Replace the Imlib2-loaded single frame with our composited first frame */
	if (w->im)
		gib_imlib_free_image_and_decache(w->im);

	w->gif = anim;
	w->im = anim->frames[0];
	w->im_w = gib_imlib_image_get_width(w->im);
	w->im_h = gib_imlib_image_get_height(w->im);

	if (anim->frame_count > 1) {
		double delay = (double)anim->delays[0] / 100.0;
		feh_add_timer(cb_gif_frame_timer, w, delay, GIF_TIMER_NAME);

		/*
		 * If the slideshow delay is shorter than the GIF's total duration,
		 * extend the slide timer so the GIF can finish at least one loop.
		 */
		if (opt.slideshow_delay > 0.0) {
			double gif_duration = 0.0;
			for (int i = 0; i < anim->frame_count; i++)
				gif_duration += (double)anim->delays[i] / 100.0;

			if (gif_duration > opt.slideshow_delay) {
				feh_add_timer(cb_slide_timer, w, gif_duration,
						"SLIDE_CHANGE");
			}
		}
	}
}

void gif_animation_stop(winwidget w)
{
	if (!w || !w->gif)
		return;

	feh_remove_timer(GIF_TIMER_NAME);

	gif_anim *anim = w->gif;
	w->gif = NULL;
	w->im = NULL;
	gif_free(anim);
}

static void cb_gif_frame_timer(void *data)
{
	winwidget w = (winwidget)data;

	if (!w->gif)
		return;

	gif_anim *anim = w->gif;

	anim->current_frame++;
	if (anim->current_frame >= anim->frame_count) {
		if (anim->loop_count > 0) {
			anim->loops_done++;
			if (anim->loops_done >= anim->loop_count)
				return;
		}
		anim->current_frame = 0;
	}

	w->im = anim->frames[anim->current_frame];
	winwidget_render_image(w, 0, 0);

	double delay = (double)anim->delays[anim->current_frame] / 100.0;
	feh_add_timer(cb_gif_frame_timer, w, delay, GIF_TIMER_NAME);
}

#endif /* HAVE_LIBGIF */
