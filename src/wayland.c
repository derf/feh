/* wayland.c

Copyright (C) 2026 Jason A. Donenfeld <Jason@zx2c4.com>. All Rights Reserved.

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

#define _GNU_SOURCE
#include "feh.h"
#include "options.h"
#include "winwidget.h"
#include "events.h"
#include "timers.h"
#include "signals.h"
#include "xdg-shell-client-protocol.h"
#include "cursor-shape-v1-client-protocol.h"
#include "viewporter-client-protocol.h"
#include "fractional-scale-v1-client-protocol.h"
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <libdecor.h>
#include <linux/input-event-codes.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>
#include <sys/mman.h>
#include <poll.h>
#include <stdint.h>
#include <locale.h>
#include <termios.h>
#include <time.h>

struct output {
	struct wl_list link;
	struct wl_output *proxy;
	uint32_t name;
	int width, height, scale, transform;
};

struct output_ref {
	struct wl_list link;
	struct output *output;
};

struct surface;
struct buffer {
	struct wl_buffer *proxy;
	void *data;
	size_t size;
	int width, height, busy;
	struct surface *owner;
};

struct surface {
	struct wl_list link, outputs;
	struct wl_surface *proxy;
	struct wl_callback *frame;
	struct buffer buffers[3];
	Imlib_Image image;
	winwidget window;
	feh_menu *menu;
	double scale;
	uint32_t preferred_scale;
	struct wp_fractional_scale_v1 *fractional_scale;
	struct wp_viewport *viewport;
	int configured, mapped, dirty, redraw;
};

struct feh_wl_window {
	struct surface surface;
	struct libdecor_frame *frame;
	int floating_width, floating_height;
	int pointer_visible;
};

struct feh_wl_popup {
	struct surface surface;
	struct xdg_surface *xdg;
	struct xdg_popup *popup;
	int grabbed;
};

static struct {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct wl_shm *shm;
	struct wp_viewporter *viewporter;
	struct wp_fractional_scale_manager_v1 *scale_manager;
	struct xdg_wm_base *shell;
	struct libdecor *decor;
	struct wl_seat *seat;
	uint32_t seat_id;
	char *seat_name;
	struct wl_pointer *pointer;
	struct wl_keyboard *keyboard;
	struct wl_touch *touch;
	struct wp_cursor_shape_manager_v1 *cursor_manager;
	struct wp_cursor_shape_device_v1 *cursor_device;
	struct wl_cursor_theme *cursor_theme;
	int cursor_scale;
	struct wl_surface *cursor_surface;
	struct surface *pointer_focus, *keyboard_focus, *touch_focus;
	uint32_t enter_serial, input_serial;
	int pointer_x, pointer_y, touch_id;
	struct {
		double value, accumulated;
		int discrete, stopped;
	} axes[2];
	struct xkb_context *xkb;
	struct xkb_keymap *keymap;
	struct xkb_state *state;
	struct xkb_compose_table *compose_table;
	struct xkb_compose_state *compose;
	uint32_t repeat_key;
	int repeating, repeat_rate, repeat_delay;
	double repeat_at;
	struct wl_list outputs, surfaces;
} wl;

static void present(struct surface *s);
static void update_cursor(void);

static double monotonic_time(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return(ts.tv_sec + ts.tv_nsec / 1e9);
}

static void buffer_destroy(struct buffer * b)
{
	if (b->proxy)
		wl_buffer_destroy(b->proxy);
	if (b->data)
		munmap(b->data, b->size);
	memset(b, 0, sizeof(*b));
	return;
}

static void buffer_release(void * data, struct wl_buffer * proxy __attribute__((unused)))
{
	struct buffer *b = data;

	b->busy = 0;
	present(b->owner);
	return;
}

static const struct wl_buffer_listener buffer_listener = {
	.release = buffer_release,
};

static void buffer_create(struct buffer * b, struct surface * s, int width, int height)
{
	int fd;
	struct wl_shm_pool *pool;
	size_t size;

	if (width <= 0 || height <= 0 || width > INT32_MAX / 4 ||
			(size_t)height > INT32_MAX / ((size_t)width * 4))
		eprintf("Wayland buffer dimensions are too large: %dx%d", width, height);
	size = (size_t)width * height * 4;

	fd = memfd_create("feh", MFD_CLOEXEC);
	if (fd < 0)
		eprintf("Cannot create Wayland shared memory:");
	if (ftruncate(fd, (off_t)size) < 0)
		eprintf("Cannot allocate Wayland shared memory:");
	b->data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (b->data == MAP_FAILED)
		eprintf("Cannot map Wayland shared memory:");
	pool = wl_shm_create_pool(wl.shm, fd, (int32_t)size);
	b->proxy = wl_shm_pool_create_buffer(pool, 0, width, height, width * 4, WL_SHM_FORMAT_XRGB8888);
	wl_shm_pool_destroy(pool);
	close(fd);
	b->size = size;
	b->width = width;
	b->height = height;
	b->owner = s;
	wl_buffer_add_listener(b->proxy, &buffer_listener, b);
	return;
}

static void frame_done(void * data, struct wl_callback * callback, uint32_t time __attribute__((unused)))
{
	struct surface *s = data;

	wl_callback_destroy(callback);
	s->frame = NULL;
	present(s);
	return;
}

static const struct wl_callback_listener frame_listener = {
	.done = frame_done,
};

static void present(struct surface * s)
{
	struct buffer *b = NULL;
	int width, height;

	if (!s->configured || !s->dirty || s->redraw || s->frame || !s->image ||
			(s->window && !s->window->visible))
		return;
	width = gib_imlib_image_get_width(s->image);
	height = gib_imlib_image_get_height(s->image);
	for (size_t i = 0; i < sizeof(s->buffers) / sizeof(s->buffers[0]); i++) {
		if (!s->buffers[i].busy) {
			b = &s->buffers[i];
			break;
		}
	}
	if (!b)
		return;
	if (b->width != width || b->height != height) {
		buffer_destroy(b);
		buffer_create(b, s, width, height);
	}
	imlib_context_set_image(s->image);
	memcpy(b->data, imlib_image_get_data_for_reading_only(), b->size);
	if (s->window) {
		struct libdecor_state *state = libdecor_state_new(s->window->w, s->window->h);
		libdecor_frame_commit(s->window->wl->frame, state, NULL);
		libdecor_state_free(state);
	}
	if (s->viewport) {
		wp_viewport_set_destination(s->viewport, s->window ? s->window->w : s->menu->w,
			s->window ? s->window->h : s->menu->h);
		wl_surface_set_buffer_scale(s->proxy, 1);
	} else
		wl_surface_set_buffer_scale(s->proxy, (int) s->scale);
	wl_surface_attach(s->proxy, b->proxy, 0, 0);
	wl_surface_damage_buffer(s->proxy, 0, 0, INT32_MAX, INT32_MAX);
	s->frame = wl_surface_frame(s->proxy);
	wl_callback_add_listener(s->frame, &frame_listener, s);
	b->busy = 1;
	s->dirty = 0;
	wl_surface_commit(s->proxy);
	s->mapped = 1;
	if (s->window && menu_root && menu_root->visible && menu_root->fehwin == s->window)
		feh_wl_menu_show(menu_root);
	else if (s->menu && s->menu->next && s->menu->next->visible)
		feh_wl_menu_show(s->menu->next);
	return;
}

static void surface_scale(struct surface * s)
{
	struct output_ref *ref;
	double scale = 1;

	wl_list_for_each(ref, &s->outputs, link) {
		if (ref->output->scale > scale)
			scale = ref->output->scale;
	}
	if (s->preferred_scale)
		scale = s->preferred_scale / 120.0;
	if (scale != s->scale) {
		s->scale = scale;
		s->redraw = 1;
		if (wl.pointer_focus == s)
			update_cursor();
	}
	return;
}

static void preferred_scale(void * data,
		struct wp_fractional_scale_v1 * fractional_scale __attribute__((unused)), uint32_t scale)
{
	struct surface *s = data;

	s->preferred_scale = scale;
	surface_scale(s);
	return;
}

static const struct wp_fractional_scale_v1_listener fractional_scale_listener = {
	.preferred_scale = preferred_scale,
};

static void surface_enter(void * data, struct wl_surface * proxy __attribute__((unused)),
		struct wl_output * output)
{
	struct surface *s = data;
	struct output_ref *ref;
	struct output *known = NULL, *candidate;

	wl_list_for_each(candidate, &wl.outputs, link) {
		if (candidate->proxy == output) {
			known = candidate;
			break;
		}
	}
	if (!known)
		return;
	wl_list_for_each(ref, &s->outputs, link) {
		if (ref->output == known)
			return;
	}
	ref = emalloc(sizeof(*ref));
	ref->output = known;
	wl_list_insert(&s->outputs, &ref->link);
	surface_scale(s);
	return;
}

static void surface_leave(void * data, struct wl_surface * proxy __attribute__((unused)),
		struct wl_output * output)
{
	struct surface *s = data;
	struct output_ref *ref, *next;

	wl_list_for_each_safe(ref, next, &s->outputs, link) {
		if (ref->output->proxy == output) {
			wl_list_remove(&ref->link);
			free(ref);
		}
	}
	surface_scale(s);
	return;
}

static const struct wl_surface_listener surface_listener = {
	.enter = surface_enter,
	.leave = surface_leave,
};

static void surface_init(struct surface * s)
{
	wl_list_init(&s->outputs);
	wl_list_insert(wl.surfaces.prev, &s->link);
	s->scale = 1;
	s->proxy = wl_compositor_create_surface(wl.compositor);
	wl_surface_add_listener(s->proxy, &surface_listener, s);
	if (wl.viewporter && wl.scale_manager) {
		s->viewport = wp_viewporter_get_viewport(wl.viewporter, s->proxy);
		s->fractional_scale = wp_fractional_scale_manager_v1_get_fractional_scale(wl.scale_manager, s->proxy);
		wp_fractional_scale_v1_add_listener(s->fractional_scale, &fractional_scale_listener, s);
	}
	return;
}

static void surface_destroy(struct surface * s)
{
	struct output_ref *ref, *next;

	if (wl.pointer_focus == s)
		wl.pointer_focus = NULL;
	if (wl.keyboard_focus == s) {
		wl.keyboard_focus = NULL;
		wl.repeating = 0;
	}
	if (wl.touch_focus == s)
		wl.touch_focus = NULL;
	if (s->frame)
		wl_callback_destroy(s->frame);
	for (size_t i = 0; i < sizeof(s->buffers) / sizeof(s->buffers[0]); i++)
		buffer_destroy(&s->buffers[i]);
	if (s->image)
		gib_imlib_free_image_and_decache(s->image);
	wl_list_for_each_safe(ref, next, &s->outputs, link) {
		wl_list_remove(&ref->link);
		free(ref);
	}
	if (s->fractional_scale)
		wp_fractional_scale_v1_destroy(s->fractional_scale);
	if (s->viewport)
		wp_viewport_destroy(s->viewport);
	wl_surface_destroy(s->proxy);
	wl_list_remove(&s->link);
	return;
}

static struct surface * find_surface(struct wl_surface * proxy)
{
	struct surface *s;

	wl_list_for_each(s, &wl.surfaces, link) {
		if (s->proxy == proxy)
			return(s);
	}
	return(NULL);
}

static void frame_configure(struct libdecor_frame * frame, struct libdecor_configuration * configuration,
		void * data)
{
	winwidget w = data;
	struct feh_wl_window *ww = w->wl;
	enum libdecor_window_state state = LIBDECOR_WINDOW_STATE_NONE;
	struct libdecor_state *content;
	int width, height;

	if (!ww->surface.configured && opt.borderless)
		libdecor_frame_set_visibility(frame, 0);
	libdecor_configuration_get_window_state(configuration, &state);
	if (!libdecor_configuration_get_content_size(configuration, frame, &width, &height)) {
		width = ww->floating_width;
		height = ww->floating_height;
	}
	if (width <= 0)
		width = w->w;
	if (height <= 0)
		height = w->h;
	w->had_resize |= width != w->w || height != w->h ||
		w->full_screen != !!(state & LIBDECOR_WINDOW_STATE_FULLSCREEN);
	w->w = width;
	w->h = height;
	w->full_screen = !!(state & LIBDECOR_WINDOW_STATE_FULLSCREEN);
	content = libdecor_state_new(width, height);
	libdecor_frame_commit(frame, content, configuration);
	libdecor_state_free(content);
	if (libdecor_frame_is_floating(frame)) {
		ww->floating_width = width;
		ww->floating_height = height;
	}

	if (ww->surface.frame) {
		wl_callback_destroy(ww->surface.frame);
		ww->surface.frame = NULL;
	}
	ww->surface.configured = 1;
	ww->surface.redraw = 1;
	return;
}

static void frame_close(struct libdecor_frame * frame __attribute__((unused)), void * data)
{
	winwidget_destroy(data);
	return;
}

static void frame_commit(struct libdecor_frame * frame __attribute__((unused)), void * data)
{
	winwidget w = data;

	wl_surface_commit(w->wl->surface.proxy);
	return;
}

static void frame_dismiss(struct libdecor_frame * frame __attribute__((unused)),
		const char * seat __attribute__((unused)), void * data)
{
	if (menu_root && menu_root->fehwin == data)
		feh_menu_hide(menu_root, True);
	return;
}

static struct libdecor_frame_interface frame_interface = {
	.configure = frame_configure,
	.close = frame_close,
	.commit = frame_commit,
	.dismiss_popup = frame_dismiss,
};

void feh_wl_create_window(winwidget w, int width, int height)
{
	static unsigned long next_id = 1;
	struct feh_wl_window *ww = emalloc(sizeof(*ww));

	memset(ww, 0, sizeof(*ww));
	w->wl = ww;
	w->win = next_id++;
	if (opt.geom_flags & WidthValue)
		width = opt.geom_w;
	if (opt.geom_flags & HeightValue)
		height = opt.geom_h;
	if (opt.screen_clip && !opt.geom_flags) {
		if (width > feh_screen_width())
			width = feh_screen_width();
		if (height > feh_screen_height())
			height = feh_screen_height();
	}
	w->w = ww->floating_width = width > 0 ? width : 1;
	w->h = ww->floating_height = height > 0 ? height : 1;
	ww->pointer_visible = !opt.hide_pointer;
	surface_init(&ww->surface);
	ww->surface.window = w;
	ww->frame = libdecor_decorate(wl.decor, ww->surface.proxy, &frame_interface, w);
	if (!ww->frame)
		eprintf("Cannot create Wayland toplevel");
	libdecor_frame_set_app_id(ww->frame, opt.x11_class ? opt.x11_class : "feh");
	libdecor_frame_set_min_content_size(ww->frame, 1, 1);
	libdecor_frame_set_visibility(ww->frame, !opt.borderless);
	feh_wl_title(w);
	if (w->full_screen)
		feh_wl_fullscreen(w, 1);
	if (opt.geom_enabled && w->type != WIN_TYPE_THUMBNAIL) {
		opt.geom_w = w->w;
		opt.geom_h = w->h;
		opt.geom_flags |= WidthValue | HeightValue;
	}
	return;
}

void feh_wl_destroy_window(winwidget w)
{
	libdecor_frame_unref(w->wl->frame);
	surface_destroy(&w->wl->surface);
	free(w->wl);
	w->wl = NULL;
	w->canvas = NULL;
	w->win = 0;
	opt.mode = MODE_NORMAL;
	return;
}

void feh_wl_title(winwidget w)
{
	if (w->wl)
		libdecor_frame_set_title(w->wl->frame, w->name ? w->name : "feh");
	return;
}

void feh_wl_show(winwidget w)
{
	if (!w->visible) {
		w->visible = 1;
		libdecor_frame_map(w->wl->frame);
	}
	return;
}

void feh_wl_hide(winwidget w)
{
	libdecor_frame_set_minimized(w->wl->frame);
	return;
}

void feh_wl_resize(winwidget w, int width, int height, int force)
{
	if (w->full_screen || (w->wl->surface.configured && !libdecor_frame_is_floating(w->wl->frame)))
		return;
	if ((opt.geom_flags & (WidthValue | HeightValue)) && !force) {
		w->had_resize = 1;
		return;
	}
	if (opt.screen_clip) {
		int max_width = feh_screen_width(), max_height = feh_screen_height();
		double zoom = w->zoom;

		if (opt.scale_down && !opt.keep_zoom_vp)
			feh_calc_needed_zoom(&zoom, w->im_w, w->im_h,
				width > max_width ? max_width : width,
				height > max_height ? max_height : height);
		width = w->im_w * zoom;
		height = w->im_h * zoom;
		if (width > max_width)
			width = max_width;
		if (height > max_height)
			height = max_height;
	}
	if (width < 1)
		width = 1;
	if (height < 1)
		height = 1;
	w->had_resize |= w->w != width || w->h != height;
	w->w = w->wl->floating_width = width;
	w->h = w->wl->floating_height = height;
	if (force && (opt.geom_flags & (WidthValue | HeightValue)) && w->type != WIN_TYPE_THUMBNAIL) {
		opt.geom_w = width;
		opt.geom_h = height;
	}
	return;
}

static struct wl_output * selected_output(void)
{
	struct output *output;
	struct wl_output *target = NULL;
	int index = 0;

	wl_list_for_each(output, &wl.outputs, link) {
		if (index++ == opt.xinerama_index)
			target = output->proxy;
	}
	return(target);
}

void feh_wl_fullscreen(winwidget w, int fullscreen)
{
	if (fullscreen)
		libdecor_frame_set_fullscreen(w->wl->frame, selected_output());
	else
		libdecor_frame_unset_fullscreen(w->wl->frame);
	return;
}

static uint32_t background_color(void)
{
	static int parsed;
	static uint32_t color;
	const char *name = opt.image_bg;

	if (parsed)
		return(color);
	parsed = 1;
	if (!name || !strcmp(name, "default") || !strcmp(name, "checks"))
		return(0);
	if (name[0] == '#') {
		char *end;
		unsigned long value = strtoul(name + 1, &end, 16);
		if (!*end && strlen(name) == 7)
			return(color = value);
		if (!*end && strlen(name) == 4)
			return(color = ((value & 0xf00) * 0x1100) | ((value & 0xf0) * 0x110) |
				((value & 0xf) * 0x11));
	} else if (!strcmp(name, "black")) {
		return(0);
	} else if (!strcmp(name, "white")) {
		return(color = 0xffffff);
	}
	weprintf("Unknown Wayland background colour '%s'; using black", name);
	return(0);
}

void feh_wl_prepare_image(winwidget w)
{
	struct surface *s = &w->wl->surface;
	double scale = s->scale;
	int width, height;
	uint32_t color = background_color();
	int checks = opt.image_bg && !strcmp(opt.image_bg, "checks");
	DATA32 *pixels;

	if (w->w < 1 || w->h < 1 || w->w > INT32_MAX / scale / 4 ||
			w->h > INT32_MAX / scale / (w->w * scale * 4))
		eprintf("Wayland image dimensions are too large");
	width = fmax(1, lround(w->w * scale));
	height = fmax(1, lround(w->h * scale));
	if (!s->image || width != gib_imlib_image_get_width(s->image) ||
			height != gib_imlib_image_get_height(s->image)) {
		if (s->image)
			gib_imlib_free_image_and_decache(s->image);
		s->image = imlib_create_image(width, height);
		if (!s->image)
			eprintf("Cannot allocate Wayland image");
	}
	w->canvas = s->image;
	w->canvas_scale = scale;
	if (!w->full_screen && (!opt.image_bg || !strcmp(opt.image_bg, "default")))
		checks = 1;
	imlib_context_set_image(s->image);
	imlib_image_set_has_alpha(0);
	pixels = imlib_image_get_data();
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			uint32_t pixel = color;
			if (checks)
				pixel = ((int) (x / (8 * scale)) ^ (int) (y / (8 * scale))) & 1 ? 0x909090 : 0x646464;
			pixels[(size_t)y * width + x] = 0xff000000 | pixel;
		}
	}
	imlib_image_put_back_data(pixels);
	return;
}

void feh_wl_present(winwidget w)
{
	w->wl->surface.dirty = 1;
	present(&w->wl->surface);
	return;
}

static void popup_configure(void * data __attribute__((unused)),
		struct xdg_popup * popup __attribute__((unused)), int32_t x __attribute__((unused)),
		int32_t y __attribute__((unused)), int32_t width __attribute__((unused)),
		int32_t height __attribute__((unused)))
{
	return;
}

static void popup_done(void * data __attribute__((unused)), struct xdg_popup * popup __attribute__((unused)))
{
	if (menu_root)
		feh_menu_hide(menu_root, True);
	return;
}

static const struct xdg_popup_listener popup_listener = {
	.configure = popup_configure,
	.popup_done = popup_done,
};

static void popup_surface_configure(void * data, struct xdg_surface * xdg, uint32_t serial)
{
	feh_menu *m = data;

	xdg_surface_ack_configure(xdg, serial);
	m->wl->surface.configured = 1;
	m->wl->surface.redraw = 1;
	return;
}

static const struct xdg_surface_listener popup_surface_listener = {
	.configure = popup_surface_configure,
};

void feh_wl_menu_show(feh_menu * m)
{
	struct xdg_positioner *positioner;
	struct xdg_surface *parent;
	int x, y;

	if (m->wl)
		return;
	if (m->prev) {
		if (!m->prev->wl || !m->prev->wl->surface.mapped)
			return;
	} else if (!m->fehwin->wl->surface.mapped)
		return;
	m->wl = emalloc(sizeof(*m->wl));
	memset(m->wl, 0, sizeof(*m->wl));
	surface_init(&m->wl->surface);
	m->wl->surface.menu = m;
	m->wl->surface.scale = m->fehwin->wl->surface.scale;
	m->wl->xdg = xdg_wm_base_get_xdg_surface(wl.shell, m->wl->surface.proxy);
	xdg_surface_add_listener(m->wl->xdg, &popup_surface_listener, m);
	positioner = xdg_wm_base_create_positioner(wl.shell);
	if (m->prev) {
		parent = m->prev->wl->xdg;
		x = 0;
		y = m->y - m->prev->y;
	} else {
		parent = libdecor_frame_get_xdg_surface(m->fehwin->wl->frame);
		x = m->x < m->fehwin->w ? m->x : m->fehwin->w - 1;
		y = m->y < m->fehwin->h ? m->y : m->fehwin->h - 1;
		libdecor_frame_translate_coordinate(m->fehwin->wl->frame, x, y, &x, &y);
	}
	xdg_positioner_set_size(positioner, m->w, m->h);
	xdg_positioner_set_anchor_rect(positioner, x, y, m->prev ? m->prev->w : 1, 1);
	xdg_positioner_set_anchor(positioner, m->prev ?
		XDG_POSITIONER_ANCHOR_TOP_RIGHT : XDG_POSITIONER_ANCHOR_TOP_LEFT);
	xdg_positioner_set_gravity(positioner, XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT);
	xdg_positioner_set_constraint_adjustment(positioner,
		XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_FLIP_X | XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_X |
		XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_Y);
	m->wl->popup = xdg_surface_get_popup(m->wl->xdg, parent, positioner);
	xdg_popup_add_listener(m->wl->popup, &popup_listener, m);
	if (wl.seat && wl.input_serial)
		xdg_popup_grab(m->wl->popup, wl.seat, wl.input_serial);
	xdg_positioner_destroy(positioner);
	wl_surface_commit(m->wl->surface.proxy);
	if (!m->prev && wl.seat_name) {
		libdecor_frame_popup_grab(m->fehwin->wl->frame, wl.seat_name);
		m->wl->grabbed = 1;
	}
	return;
}

void feh_wl_menu_hide(feh_menu * m)
{
	if (!m->wl)
		return;
	if (m->wl->grabbed)
		libdecor_frame_popup_ungrab(m->fehwin->wl->frame, wl.seat_name);
	xdg_popup_destroy(m->wl->popup);
	xdg_surface_destroy(m->wl->xdg);
	surface_destroy(&m->wl->surface);
	free(m->wl);
	m->wl = NULL;
	return;
}

void feh_wl_menu_redraw(feh_menu * m)
{
	struct surface *s;

	if (!m->wl)
		return;
	s = &m->wl->surface;
	if (s->image)
		gib_imlib_free_image_and_decache(s->image);
	s->image = imlib_create_image(fmax(1, lround(m->w * s->scale)),
		fmax(1, lround(m->h * s->scale)));
	if (!s->image)
		eprintf("Cannot allocate Wayland menu image");
	feh_menu_draw_to_buf(m, s->image, 0, 0, s->scale);
	s->dirty = 1;
	present(s);
	return;
}

static unsigned int modifiers(void)
{
	unsigned int state = 0;

	if (!wl.state)
		return(0);
	if (xkb_state_mod_name_is_active(wl.state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE) > 0)
		state |= ShiftMask;
	if (xkb_state_mod_name_is_active(wl.state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE) > 0)
		state |= ControlMask;
	if (xkb_state_mod_name_is_active(wl.state, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE) > 0)
		state |= Mod1Mask;
	if (xkb_state_mod_name_is_active(wl.state, XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE) > 0)
		state |= Mod4Mask;
	return(state);
}

static void pointer_event(struct surface * s, int type, unsigned int button, int x, int y)
{
	XEvent event;

	if (menu_root && (!s || !s->menu)) {
		if (type == ButtonPress)
			feh_menu_hide(menu_root, True);
		return;
	}
	if (!s)
		return;
	if (s->menu) {
		feh_menu *m = s->menu;
		if (type == MotionNotify) {
			feh_menu_item *item = feh_menu_find_at_xy(m, x, y);
			if (item != feh_menu_find_selected(m)) {
				feh_menu_deselect_selected(m);
				if (item && (item->action || item->submenu || item->func_gen_sub))
					feh_menu_select(m, item);
			}
		} else if (type == ButtonRelease) {
			feh_menu_item *item = feh_menu_find_at_xy(m, x, y);
			feh_menu_item_activate(m, item);
		}
		return;
	}
	memset(&event, 0, sizeof(event));
	event.type = type;
	if (type == MotionNotify) {
		event.xmotion.window = s->window->win;
		event.xmotion.x = x;
		event.xmotion.y = y;
		event.xmotion.state = modifiers();
	} else {
		event.xbutton.window = s->window->win;
		event.xbutton.x = x;
		event.xbutton.y = y;
		event.xbutton.button = button;
		event.xbutton.state = modifiers();
	}
	if (ev_handler[type])
		ev_handler[type](&event);
	return;
}

static void update_cursor(void)
{
	struct wl_cursor *cursor;
	struct wl_cursor_image *image;
	int scale, size = 24;
	char *configured_size;

	if (!wl.pointer || !wl.pointer_focus)
		return;
	if (wl.pointer_focus->window && !wl.pointer_focus->window->wl->pointer_visible) {
		wl_pointer_set_cursor(wl.pointer, wl.enter_serial, NULL, 0, 0);
		return;
	}
	if (wl.cursor_device) {
		wp_cursor_shape_device_v1_set_shape(wl.cursor_device, wl.enter_serial,
			WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT);
		return;
	}
	scale = ceil(wl.pointer_focus->scale);
	if (wl.cursor_scale != scale) {
		if (wl.cursor_theme)
			wl_cursor_theme_destroy(wl.cursor_theme);
		configured_size = getenv("XCURSOR_SIZE");
		if (configured_size && atoi(configured_size) > 0)
			size = atoi(configured_size);
		if (size > INT_MAX / scale)
			return;
		wl.cursor_theme = wl_cursor_theme_load(getenv("XCURSOR_THEME"), size * scale, wl.shm);
		wl.cursor_scale = scale;
	}
	if (!wl.cursor_theme)
		return;
	cursor = wl_cursor_theme_get_cursor(wl.cursor_theme, "left_ptr");
	if (!cursor)
		cursor = wl_cursor_theme_get_cursor(wl.cursor_theme, "default");
	if (!cursor || !cursor->image_count)
		return;
	image = cursor->images[0];
	wl_pointer_set_cursor(wl.pointer, wl.enter_serial, wl.cursor_surface,
		image->hotspot_x / scale, image->hotspot_y / scale);
	wl_surface_set_buffer_scale(wl.cursor_surface, scale);
	wl_surface_attach(wl.cursor_surface, wl_cursor_image_get_buffer(image), 0, 0);
	wl_surface_damage_buffer(wl.cursor_surface, 0, 0, image->width, image->height);
	wl_surface_commit(wl.cursor_surface);
	return;
}

void feh_wl_set_pointer(winwidget w, int visible)
{
	w->wl->pointer_visible = visible;
	update_cursor();
	return;
}

void feh_wl_pointer_position(winwidget w, int * x, int * y)
{
	if (wl.pointer_focus == &w->wl->surface) {
		*x = wl.pointer_x;
		*y = wl.pointer_y;
	} else {
		*x = w->w / 2;
		*y = w->h / 2;
	}
	return;
}

static void pointer_enter(void * data __attribute__((unused)),
		struct wl_pointer * pointer __attribute__((unused)), uint32_t serial,
		struct wl_surface * surface, wl_fixed_t x, wl_fixed_t y)
{
	wl.pointer_focus = find_surface(surface);
	wl.pointer_x = wl_fixed_to_int(x);
	wl.pointer_y = wl_fixed_to_int(y);
	wl.enter_serial = serial;
	update_cursor();
	return;
}

static void cancel_drag(struct surface * s)
{
	if (s && s->window && s->window->mode != MODE_NORMAL) {
		s->window->mode = MODE_NORMAL;
		opt.mode = MODE_NORMAL;
		s->redraw = 1;
	}
	return;
}

static void pointer_leave(void * data __attribute__((unused)),
		struct wl_pointer * pointer __attribute__((unused)), uint32_t serial __attribute__((unused)),
		struct wl_surface * surface __attribute__((unused)))
{
	if (!wl.touch_focus)
		cancel_drag(wl.pointer_focus);
	wl.pointer_focus = NULL;
	memset(wl.axes, 0, sizeof(wl.axes));
	return;
}

static void pointer_motion(void * data __attribute__((unused)),
		struct wl_pointer * pointer __attribute__((unused)), uint32_t time __attribute__((unused)),
		wl_fixed_t x, wl_fixed_t y)
{
	wl.pointer_x = wl_fixed_to_int(x);
	wl.pointer_y = wl_fixed_to_int(y);
	pointer_event(wl.pointer_focus, MotionNotify, 0, wl.pointer_x, wl.pointer_y);
	return;
}

static void pointer_button(void * data __attribute__((unused)),
		struct wl_pointer * pointer __attribute__((unused)), uint32_t serial,
		uint32_t time __attribute__((unused)), uint32_t button, uint32_t state)
{
	unsigned int number;

	if (state == WL_POINTER_BUTTON_STATE_PRESSED)
		wl.input_serial = serial;

	switch (button) {
	case BTN_LEFT:
		number = 1;
		break;
	case BTN_RIGHT:
		number = 3;
		break;
	case BTN_MIDDLE:
		number = 2;
		break;
	default:
		number = button >= BTN_SIDE ? button - BTN_SIDE + 8 : 0;
		break;
	}
	if (number)
		pointer_event(wl.pointer_focus,
				state == WL_POINTER_BUTTON_STATE_PRESSED ? ButtonPress : ButtonRelease, number,
				wl.pointer_x, wl.pointer_y);
	return;
}

static void pointer_axis(void * data __attribute__((unused)),
		struct wl_pointer * pointer __attribute__((unused)), uint32_t time __attribute__((unused)),
		uint32_t axis, wl_fixed_t value)
{
	if (axis <= WL_POINTER_AXIS_HORIZONTAL_SCROLL)
		wl.axes[axis].value += wl_fixed_to_double(value);
	return;
}

static void pointer_frame(void * data __attribute__((unused)),
		struct wl_pointer * pointer __attribute__((unused)))
{
	for (int axis = 0; axis <= WL_POINTER_AXIS_HORIZONTAL_SCROLL; axis++) {
		int steps = wl.axes[axis].discrete;
		unsigned int button;

		if (!steps) {
			wl.axes[axis].accumulated += wl.axes[axis].value;
			steps = wl.axes[axis].accumulated / 10.0;
			wl.axes[axis].accumulated -= steps * 10.0;
		}
		if (wl.axes[axis].stopped || wl.axes[axis].discrete)
			wl.axes[axis].accumulated = 0;
		wl.axes[axis].value = 0;
		wl.axes[axis].discrete = 0;
		wl.axes[axis].stopped = 0;
		button = axis == WL_POINTER_AXIS_VERTICAL_SCROLL ? (steps > 0 ? 5 : 4) : (steps > 0 ? 7 : 6);
		for (int i = 0; i < abs(steps); i++) {
			pointer_event(wl.pointer_focus, ButtonPress, button, wl.pointer_x, wl.pointer_y);
			pointer_event(wl.pointer_focus, ButtonRelease, button, wl.pointer_x, wl.pointer_y);
		}
	}
	return;
}

static void pointer_axis_source(void * data __attribute__((unused)),
		struct wl_pointer * pointer __attribute__((unused)), uint32_t source __attribute__((unused)))
{
	return;
}

static void pointer_axis_stop(void * data __attribute__((unused)),
		struct wl_pointer * pointer __attribute__((unused)), uint32_t time __attribute__((unused)),
		uint32_t axis)
{
	if (axis <= WL_POINTER_AXIS_HORIZONTAL_SCROLL)
		wl.axes[axis].stopped = 1;
	return;
}

static void pointer_axis_discrete(void * data __attribute__((unused)),
		struct wl_pointer * pointer __attribute__((unused)), uint32_t axis, int32_t discrete)
{
	if (axis <= WL_POINTER_AXIS_HORIZONTAL_SCROLL)
		wl.axes[axis].discrete += discrete;
	return;
}

static const struct wl_pointer_listener pointer_listener = {
	.enter = pointer_enter,
	.leave = pointer_leave,
	.motion = pointer_motion,
	.button = pointer_button,
	.axis = pointer_axis,
	.frame = pointer_frame,
	.axis_source = pointer_axis_source,
	.axis_stop = pointer_axis_stop,
	.axis_discrete = pointer_axis_discrete,
};

static void keyboard_keymap(void * data __attribute__((unused)),
		struct wl_keyboard * keyboard __attribute__((unused)), uint32_t format, int32_t fd,
		uint32_t size)
{
	char *map;
	struct xkb_keymap *keymap = NULL;
	struct xkb_state *state;

	if (format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1 && size) {
		map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
		if (map != MAP_FAILED) {
			if (map[size - 1] == '\0')
				keymap = xkb_keymap_new_from_string(wl.xkb, map,
					XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
			munmap(map, size);
		}
	}
	close(fd);
	state = keymap ? xkb_state_new(keymap) : NULL;
	if (keymap && !state) {
		xkb_keymap_unref(keymap);
		keymap = NULL;
	}
	xkb_state_unref(wl.state);
	xkb_keymap_unref(wl.keymap);
	wl.keymap = keymap;
	wl.state = state;
	wl.repeating = 0;
	if (wl.compose)
		xkb_compose_state_reset(wl.compose);
	return;
}

static void keyboard_enter(void * data __attribute__((unused)),
		struct wl_keyboard * keyboard __attribute__((unused)),
		uint32_t serial __attribute__((unused)), struct wl_surface * surface,
		struct wl_array * keys __attribute__((unused)))
{
	wl.keyboard_focus = find_surface(surface);
	wl.repeating = 0;
	if (wl.compose)
		xkb_compose_state_reset(wl.compose);
	return;
}

static void keyboard_leave(void * data __attribute__((unused)),
		struct wl_keyboard * keyboard __attribute__((unused)),
		uint32_t serial __attribute__((unused)), struct wl_surface * surface __attribute__((unused)))
{
	wl.keyboard_focus = NULL;
	wl.repeating = 0;
	if (wl.compose)
		xkb_compose_state_reset(wl.compose);
	return;
}

static void emit_key(uint32_t key)
{
	char text[128];
	int length;
	unsigned int state = modifiers();
	xkb_keysym_t sym;
	winwidget w;

	if ((!wl.keyboard_focus && !menu_root) || !wl.state)
		return;
	w = menu_root ? menu_root->fehwin : wl.keyboard_focus->window;
	if (!w && wl.keyboard_focus->menu)
		w = wl.keyboard_focus->menu->fehwin;
	sym = xkb_state_key_get_one_sym(wl.state, key);
	length = xkb_state_key_get_utf8(wl.state, key, text, sizeof(text));
	if (wl.compose && w && w->caption_entry && !menu_root) {
		xkb_compose_state_feed(wl.compose, sym);
		switch (xkb_compose_state_get_status(wl.compose)) {
		case XKB_COMPOSE_COMPOSING:
			return;
		case XKB_COMPOSE_COMPOSED:
			sym = xkb_compose_state_get_one_sym(wl.compose);
			length = xkb_compose_state_get_utf8(wl.compose, text, sizeof(text));
			xkb_compose_state_reset(wl.compose);
			break;
		case XKB_COMPOSE_CANCELLED:
			xkb_compose_state_reset(wl.compose);
			return;
		default:
			break;
		}
	}
	if (length < 0 || (size_t)length >= sizeof(text))
		length = 0;
	text[length] = '\0';

	if (sym < 128 && isprint(sym) && !isspace(sym))
		state &= ~ShiftMask;
	if (menu_root)
		feh_event_handle_menu_key(state, sym);
	else if (w)
		feh_event_handle_generic(w, state, sym, 0, text, length);
	return;
}

static void keyboard_key(void * data __attribute__((unused)),
		struct wl_keyboard * keyboard __attribute__((unused)), uint32_t serial,
		uint32_t time __attribute__((unused)), uint32_t key, uint32_t state)
{
	key += 8;
	if (state == WL_KEYBOARD_KEY_STATE_RELEASED) {
		if (key == wl.repeat_key)
			wl.repeating = 0;
		return;
	}
	if (!wl.keymap)
		return;
	wl.input_serial = serial;
	if (wl.repeat_rate > 0 && xkb_keymap_key_repeats(wl.keymap, key)) {
		wl.repeating = 1;
		wl.repeat_key = key;
		wl.repeat_at = monotonic_time() + wl.repeat_delay / 1000.0;
	}
	emit_key(key);
	return;
}

static void keyboard_modifiers(void * data __attribute__((unused)),
		struct wl_keyboard * keyboard __attribute__((unused)),
		uint32_t serial __attribute__((unused)), uint32_t depressed, uint32_t latched,
		uint32_t locked, uint32_t group)
{
	if (wl.state)
		xkb_state_update_mask(wl.state, depressed, latched, locked, 0, 0, group);
	return;
}

static void keyboard_repeat_info(void * data __attribute__((unused)),
		struct wl_keyboard * keyboard __attribute__((unused)), int32_t rate, int32_t delay)
{
	wl.repeat_rate = rate > 0 ? rate : 0;
	wl.repeat_delay = delay > 0 ? delay : 0;
	if (!wl.repeat_rate)
		wl.repeating = 0;
	return;
}

static const struct wl_keyboard_listener keyboard_listener = {
	.keymap = keyboard_keymap,
	.enter = keyboard_enter,
	.leave = keyboard_leave,
	.key = keyboard_key,
	.modifiers = keyboard_modifiers,
	.repeat_info = keyboard_repeat_info,
};

static void touch_down(void * data __attribute__((unused)), struct wl_touch * touch __attribute__((unused)),
		uint32_t serial, uint32_t time __attribute__((unused)), struct wl_surface * surface,
		int32_t id, wl_fixed_t x, wl_fixed_t y)
{
	if (wl.touch_focus)
		return;
	wl.input_serial = serial;
	wl.touch_focus = find_surface(surface);
	wl.touch_id = id;
	wl.pointer_x = wl_fixed_to_int(x);
	wl.pointer_y = wl_fixed_to_int(y);
	pointer_event(wl.touch_focus, ButtonPress, 1, wl.pointer_x, wl.pointer_y);
	return;
}

static void touch_up(void * data __attribute__((unused)), struct wl_touch * touch __attribute__((unused)),
		uint32_t serial __attribute__((unused)), uint32_t time __attribute__((unused)), int32_t id)
{
	if (id != wl.touch_id)
		return;
	pointer_event(wl.touch_focus, ButtonRelease, 1, wl.pointer_x, wl.pointer_y);
	wl.touch_focus = NULL;
	return;
}

static void touch_motion(void * data __attribute__((unused)),
		struct wl_touch * touch __attribute__((unused)), uint32_t time __attribute__((unused)),
		int32_t id, wl_fixed_t x, wl_fixed_t y)
{
	if (id != wl.touch_id)
		return;
	wl.pointer_x = wl_fixed_to_int(x);
	wl.pointer_y = wl_fixed_to_int(y);
	pointer_event(wl.touch_focus, MotionNotify, 0, wl.pointer_x, wl.pointer_y);
	return;
}

static void touch_frame(void * data __attribute__((unused)), struct wl_touch * touch __attribute__((unused)))
{
	return;
}

static void touch_cancel(void * data __attribute__((unused)), struct wl_touch * touch __attribute__((unused)))
{
	cancel_drag(wl.touch_focus);
	wl.touch_focus = NULL;
	return;
}

static const struct wl_touch_listener touch_listener = {
	.down = touch_down,
	.up = touch_up,
	.motion = touch_motion,
	.frame = touch_frame,
	.cancel = touch_cancel,
};

static void seat_capabilities(void * data __attribute__((unused)), struct wl_seat * seat,
		uint32_t capabilities)
{
	if ((capabilities & WL_SEAT_CAPABILITY_POINTER) && !wl.pointer) {
		wl.pointer = wl_seat_get_pointer(seat);
		wl_pointer_add_listener(wl.pointer, &pointer_listener, NULL);
		if (wl.cursor_manager)
			wl.cursor_device = wp_cursor_shape_manager_v1_get_pointer(wl.cursor_manager, wl.pointer);
	} else if (!(capabilities & WL_SEAT_CAPABILITY_POINTER) && wl.pointer) {
		if (wl.cursor_device) {
			wp_cursor_shape_device_v1_destroy(wl.cursor_device);
			wl.cursor_device = NULL;
		}
		pointer_leave(NULL, wl.pointer, 0, NULL);
		wl_pointer_release(wl.pointer);
		wl.pointer = NULL;
	}
	if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && !wl.keyboard) {
		wl.keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(wl.keyboard, &keyboard_listener, NULL);
	} else if (!(capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && wl.keyboard) {
		wl_keyboard_release(wl.keyboard);
		wl.keyboard = NULL;
		wl.keyboard_focus = NULL;
		wl.repeating = 0;
	}
	if ((capabilities & WL_SEAT_CAPABILITY_TOUCH) && !wl.touch) {
		wl.touch = wl_seat_get_touch(seat);
		wl_touch_add_listener(wl.touch, &touch_listener, NULL);
	} else if (!(capabilities & WL_SEAT_CAPABILITY_TOUCH) && wl.touch) {
		touch_cancel(NULL, wl.touch);
		wl_touch_release(wl.touch);
		wl.touch = NULL;
	}
	return;
}

static void seat_name(void * data __attribute__((unused)), struct wl_seat * seat __attribute__((unused)),
		const char * name)
{
	free(wl.seat_name);
	wl.seat_name = estrdup((char *) name);
	return;
}

static const struct wl_seat_listener seat_listener = {
	.capabilities = seat_capabilities,
	.name = seat_name,
};

static void output_geometry(void * data, struct wl_output * proxy __attribute__((unused)),
		int32_t x __attribute__((unused)), int32_t y __attribute__((unused)),
		int32_t physical_width __attribute__((unused)),
		int32_t physical_height __attribute__((unused)), int32_t subpixel __attribute__((unused)),
		const char * make __attribute__((unused)), const char * model __attribute__((unused)),
		int32_t transform)
{
	struct output *output = data;

	output->transform = transform;
	return;
}

static void output_mode(void * data, struct wl_output * proxy __attribute__((unused)), uint32_t flags,
		int32_t width, int32_t height, int32_t refresh __attribute__((unused)))
{
	struct output *output = data;

	if (flags & WL_OUTPUT_MODE_CURRENT) {
		output->width = width;
		output->height = height;
	}
	return;
}

static void output_done(void * data __attribute__((unused)), struct wl_output * proxy __attribute__((unused)))
{
	struct surface *s;

	wl_list_for_each(s, &wl.surfaces, link)
		surface_scale(s);
	return;
}

static void output_scale(void * data, struct wl_output * proxy __attribute__((unused)), int32_t factor)
{
	struct output *output = data;

	output->scale = factor > 0 ? factor : 1;
	return;
}

static const struct wl_output_listener output_listener = {
	.geometry = output_geometry,
	.mode = output_mode,
	.done = output_done,
	.scale = output_scale,
};

void feh_wl_screen_size(int * width, int * height)
{
	struct output *output, *selected = NULL;
	int index = 0;

	*width = INT_MAX;
	*height = INT_MAX;
	wl_list_for_each(output, &wl.outputs, link) {
		if (!selected || index == opt.xinerama_index)
			selected = output;
		index++;
	}
	if (selected && selected->width > 0 && selected->height > 0) {
		*width = selected->width / selected->scale;
		*height = selected->height / selected->scale;
		if (selected->transform & 1) {
			int swap = *width;
			*width = *height;
			*height = swap;
		}
	}
	return;
}

static void shell_ping(void * data __attribute__((unused)), struct xdg_wm_base * shell, uint32_t serial)
{
	xdg_wm_base_pong(shell, serial);
	return;
}

static const struct xdg_wm_base_listener shell_listener = {
	.ping = shell_ping,
};

static void registry_global(void * data __attribute__((unused)), struct wl_registry * registry,
		uint32_t name, const char * interface, uint32_t version)
{
	if (!strcmp(interface, wl_compositor_interface.name) && version >= 4)
		wl.compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
	else if (!strcmp(interface, wl_shm_interface.name))
		wl.shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	else if (!strcmp(interface, xdg_wm_base_interface.name)) {
		wl.shell = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
		xdg_wm_base_add_listener(wl.shell, &shell_listener, NULL);
	} else if (!strcmp(interface, wl_seat_interface.name) && version >= 5 && !wl.seat) {
		wl.seat_id = name;
		wl.seat = wl_registry_bind(registry, name, &wl_seat_interface, 5);
		wl_seat_add_listener(wl.seat, &seat_listener, NULL);
	} else if (!strcmp(interface, wp_viewporter_interface.name)) {
		wl.viewporter = wl_registry_bind(registry, name, &wp_viewporter_interface, 1);
	} else if (!strcmp(interface, wp_fractional_scale_manager_v1_interface.name)) {
		wl.scale_manager = wl_registry_bind(registry, name, &wp_fractional_scale_manager_v1_interface, 1);
	} else if (!strcmp(interface, wp_cursor_shape_manager_v1_interface.name)) {
		wl.cursor_manager = wl_registry_bind(registry, name, &wp_cursor_shape_manager_v1_interface, 1);
		if (wl.pointer)
			wl.cursor_device = wp_cursor_shape_manager_v1_get_pointer(wl.cursor_manager, wl.pointer);
	} else if (!strcmp(interface, wl_output_interface.name) && version >= 2) {
		struct output *output = emalloc(sizeof(*output));
		memset(output, 0, sizeof(*output));
		output->name = name;
		output->scale = 1;
		output->proxy = wl_registry_bind(registry, name, &wl_output_interface, 2);
		wl_list_insert(wl.outputs.prev, &output->link);
		wl_output_add_listener(output->proxy, &output_listener, output);
	}
	return;
}

static void registry_remove(void * data __attribute__((unused)),
		struct wl_registry * registry __attribute__((unused)), uint32_t name)
{
	struct output *output, *next;

	wl_list_for_each_safe(output, next, &wl.outputs, link) {
		if (output->name == name) {
			struct surface *s;
			wl_list_for_each(s, &wl.surfaces, link)
				surface_leave(s, s->proxy, output->proxy);
			wl_output_destroy(output->proxy);
			wl_list_remove(&output->link);
			free(output);
		}
	}
	if (wl.seat && name == wl.seat_id) {
		if (menu_root)
			feh_menu_hide(menu_root, True);
		seat_capabilities(NULL, wl.seat, 0);
		wl_seat_destroy(wl.seat);
		wl.seat = NULL;
		free(wl.seat_name);
		wl.seat_name = NULL;
		wl.input_serial = 0;
	}
	return;
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_global,
	.global_remove = registry_remove,
};

static void decor_error(struct libdecor * decor __attribute__((unused)),
		enum libdecor_error error __attribute__((unused)), const char * message)
{
	eprintf("Wayland window decoration error: %s", message);
	return;
}

static struct libdecor_interface decor_interface = {
	.error = decor_error,
};

int feh_wl_init(void)
{
	wl_list_init(&wl.outputs);
	wl_list_init(&wl.surfaces);
	wl.display = wl_display_connect(NULL);
	if (!wl.display)
		return(0);
	wl.xkb = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (!wl.xkb)
		eprintf("Cannot initialize xkbcommon");
	wl.compose_table = xkb_compose_table_new_from_locale(wl.xkb, setlocale(LC_CTYPE, NULL),
		XKB_COMPOSE_COMPILE_NO_FLAGS);
	if (wl.compose_table)
		wl.compose = xkb_compose_state_new(wl.compose_table, XKB_COMPOSE_STATE_NO_FLAGS);
	wl.registry = wl_display_get_registry(wl.display);
	wl_registry_add_listener(wl.registry, &registry_listener, NULL);
	if (wl_display_roundtrip(wl.display) < 0 || wl_display_roundtrip(wl.display) < 0)
		eprintf("Lost Wayland connection during initialization");
	if (!wl.compositor || !wl.shm || !wl.shell)
		eprintf("Wayland compositor must provide wl_compositor v4, wl_shm, and xdg_wm_base");
	wl.cursor_surface = wl_compositor_create_surface(wl.compositor);
	wl.decor = libdecor_new(wl.display, &decor_interface);
	if (!wl.decor)
		eprintf("Cannot initialize Wayland window decorations");
	return(1);
}

void feh_wl_shutdown(void)
{
	struct output *output, *next;

	if (!wl.display)
		return;
	seat_capabilities(NULL, wl.seat, 0);
	if (wl.seat)
		wl_seat_destroy(wl.seat);
	free(wl.seat_name);
	if (wl.cursor_manager)
		wp_cursor_shape_manager_v1_destroy(wl.cursor_manager);
	if (wl.cursor_surface)
		wl_surface_destroy(wl.cursor_surface);
	if (wl.cursor_theme)
		wl_cursor_theme_destroy(wl.cursor_theme);
	if (wl.decor)
		libdecor_unref(wl.decor);
	wl_list_for_each_safe(output, next, &wl.outputs, link) {
		wl_output_destroy(output->proxy);
		wl_list_remove(&output->link);
		free(output);
	}
	if (wl.shell)
		xdg_wm_base_destroy(wl.shell);
	if (wl.scale_manager)
		wp_fractional_scale_manager_v1_destroy(wl.scale_manager);
	if (wl.viewporter)
		wp_viewporter_destroy(wl.viewporter);
	if (wl.shm)
		wl_shm_destroy(wl.shm);
	if (wl.compositor)
		wl_compositor_destroy(wl.compositor);
	if (wl.registry)
		wl_registry_destroy(wl.registry);
	xkb_compose_state_unref(wl.compose);
	xkb_compose_table_unref(wl.compose_table);
	xkb_state_unref(wl.state);
	xkb_keymap_unref(wl.keymap);
	xkb_context_unref(wl.xkb);
	wl_display_disconnect(wl.display);
	wl.display = NULL;
	return;
}

static void dispatch_pending(void)
{
	if (wl_display_dispatch_pending(wl.display) < 0)
		eprintf("Connection to Wayland display lost");
	if (libdecor_dispatch(wl.decor, 0) < 0 && errno != EINTR)
		eprintf("Dispatching Wayland decoration events failed:");
	return;
}

int feh_wl_iteration(int block)
{
	static int initialized;
	static double previous;
	static int timers_running;
	struct pollfd fds[4];
	int count = 2, stdin_index = -1, result, poll_errno;
#ifdef HAVE_INOTIFY
	int inotify_index = -1;
#endif
	double now, elapsed, timeout = -1;
	struct timespec wait_time, *wait_ptr = NULL;
	sigset_t signals, old_signals;
	struct surface *s;

	if (!initialized) {
		initialized = 1;
		previous = monotonic_time();
		if (isatty(STDIN_FILENO) && !opt.multiwindow && getpgrp() == tcgetpgrp(STDIN_FILENO))
			setup_stdin();
	}
	dispatch_pending();
	if (!window_num || sig_exit)
		return(0);
	now = monotonic_time();
	elapsed = now - previous;
	previous = now;
	if (first_timer && opt.mode == MODE_NORMAL && !opt.paused) {
		if (first_timer->just_added)
			first_timer->just_added = 0;
		else if (timers_running)
			first_timer->in -= elapsed;
		timeout = fmax(0, first_timer->in);
		if (first_timer->in <= 0)
			feh_handle_timer();
	}
	timers_running = opt.mode == MODE_NORMAL && !opt.paused;
	if (wl.repeating && wl.keyboard_focus && wl.repeat_rate > 0) {
		if (now >= wl.repeat_at) {
			wl.repeat_at = now + 1.0 / wl.repeat_rate;
			emit_key(wl.repeat_key);
		}
		if (wl.repeating && (timeout < 0 || wl.repeat_at - now < timeout))
			timeout = fmax(0, wl.repeat_at - now);
	}
	if (!window_num || sig_exit)
		return(0);
	wl_list_for_each(s, &wl.surfaces, link) {
		if (s->redraw) {
			s->redraw = 0;
			if (s->window)
				winwidget_render_image(s->window, 0, 0);
			else if (s->menu)
				feh_wl_menu_redraw(s->menu);
		}
	}
	feh_redraw_menus();

	if (wl_display_prepare_read(wl.display) != 0) {
		dispatch_pending();
		return(window_num && !sig_exit);
	}
	memset(fds, 0, sizeof(fds));
	fds[0].fd = wl_display_get_fd(wl.display);
	fds[0].events = POLLIN;
	fds[1].fd = libdecor_get_fd(wl.decor);
	fds[1].events = POLLIN;
	if (wl_display_flush(wl.display) < 0) {
		if (errno == EAGAIN)
			fds[0].events |= POLLOUT;
		else {
			wl_display_cancel_read(wl.display);
			eprintf("Connection to Wayland display lost");
		}
	}
	if (control_via_stdin) {
		stdin_index = count;
		fds[count].fd = STDIN_FILENO;
		fds[count].events = POLLIN;
		count++;
	}
#ifdef HAVE_INOTIFY
	if (opt.auto_reload) {
		inotify_index = count;
		fds[count].fd = opt.inotify_fd;
		fds[count].events = POLLIN;
		count++;
	}
#endif

	sigemptyset(&signals);
	sigaddset(&signals, SIGUSR1);
	sigaddset(&signals, SIGUSR2);
	sigaddset(&signals, SIGTERM);
	sigaddset(&signals, SIGINT);
	sigaddset(&signals, SIGQUIT);
	sigprocmask(SIG_BLOCK, &signals, &old_signals);
	if (!block || sig_received || sig_exit)
		timeout = 0;
	if (timeout >= 0) {
		timeout = fmax(0, timeout - (monotonic_time() - now));
		wait_time.tv_sec = (time_t)timeout;
		wait_time.tv_nsec = (long)((timeout - wait_time.tv_sec) * 1e9);
		wait_ptr = &wait_time;
	}
	result = ppoll(fds, count, wait_ptr, &old_signals);
	poll_errno = errno;
	if (result > 0 && (fds[0].revents & POLLIN)) {
		if (wl_display_read_events(wl.display) < 0) {
			sigprocmask(SIG_SETMASK, &old_signals, NULL);
			eprintf("Connection to Wayland display lost");
		}
	} else
		wl_display_cancel_read(wl.display);
	sigprocmask(SIG_SETMASK, &old_signals, NULL);
	if (result < 0 && poll_errno != EINTR) {
		errno = poll_errno;
		eprintf("Waiting for Wayland events failed:");
	}
	if (result > 0) {
		if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL))
			eprintf("Connection to Wayland display lost");
		if (stdin_index >= 0 && (fds[stdin_index].revents & (POLLIN | POLLHUP)))
			feh_event_handle_stdin();
#ifdef HAVE_INOTIFY
		if (inotify_index >= 0 && (fds[inotify_index].revents & POLLIN))
			feh_event_handle_inotify();
#endif
	}
	return(window_num && !sig_exit);
}
