/* list.c

Copyright (C) 1999-2003 Tom Gilbert.
Copyright (C) 2010-2020 Birte Kristina Friesel.

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

#include "feh.h"
#include "filelist.h"
#include "options.h"

void init_list_mode(void)
{
	gib_list *l;
	feh_file *file = NULL;
	int j = 0;

	mode = "list";

	if (!opt.customlist)
		fputs("NUM\tFORMAT\tWIDTH\tHEIGHT\tPIXELS\tSIZE\tALPHA\tFILENAME\n",
				stdout);

	for (l = filelist; l; l = l->next) {
		file = FEH_FILE(l->data);
		if (opt.customlist)
			printf("%s\n", feh_printf(opt.customlist, file, NULL));
		else {
			printf("%d\t%s\t%d\t%d\t%s", ++j,
					file->info->format, file->info->width,
					file->info->height,
					format_size(file->info->pixels));
			printf("\t%s\t%c\t%s\n",
					format_size(file->size),
					file->info->has_alpha ? 'X' : '-', file->filename);
		}

		feh_action_run(file, opt.actions[0], NULL);
	}
	exit(0);
}

void init_loadables_mode(void)
{
	mode = "loadables";
	real_loadables_mode(1);
	return;
}

void init_unloadables_mode(void)
{
	mode = "unloadables";
	real_loadables_mode(0);
	return;
}

void real_loadables_mode(int loadable)
{
	feh_file *file;
	gib_list *l;
	char ret = 0;

	opt.quiet = 1;

	for (l = filelist; l; l = l->next) {
		Imlib_Image im = NULL;

		file = FEH_FILE(l->data);

		if (feh_load_image(&im, file)) {
			/* loaded ok */
			if (loadable) {
				if (opt.verbose)
					feh_display_status('.');
				puts(file->filename);
				fflush(stdout);
				feh_action_run(file, opt.actions[0], NULL);
			}
			else {
				if (opt.verbose)
					feh_display_status('s');
				ret = 1;
			}
			gib_imlib_free_image_and_decache(im);
		} else {
			/* Oh dear. */
			if (!loadable) {
				if (opt.verbose)
					feh_display_status('.');
				puts(file->filename);
				fflush(stdout);
				feh_action_run(file, opt.actions[0], NULL);
			}
			else {
				if (opt.verbose)
					feh_display_status('s');
				ret = 1;
			}
		}
	}
	if (opt.verbose)
		feh_display_status(0);
	exit(ret);
}
