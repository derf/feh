/* multiwindow.c

Copyright (C) 1999-2003 Tom Gilbert.

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
#include "winwidget.h"
#include "timers.h"
#include "filelist.h"
#include "options.h"
#include "signals.h"

void init_multiwindow_mode(void)
{
	winwidget w = NULL;
	gib_list *l;

	if (!opt.title)
		opt.title = PACKAGE " - %f";

	mode = "multiwindow";

	for (l = filelist; l; l = l->next) {
		if ((w = winwidget_create_from_file(l, WIN_TYPE_SINGLE)) != NULL) {
			winwidget_show(w);
			if (opt.reload > 0)
				feh_add_unique_timer(cb_reload_timer, w, opt.reload);
			if (!feh_main_iteration(0))
				exit(0);
		} else {
			D(("EEEK. Couldn't load image in multiwindow mode. "
						"I 'm not sure if this is a problem\n"));
		}
	}

	return;
}
