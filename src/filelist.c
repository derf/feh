/* filelist.c

Copyright (C) 1999-2003 Tom Gilbert.
Copyright (C) 2010-2011 Daniel Friesel.
Copyright (C) 2012      Christopher Hrabak

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

May 5, 2012 HRABAK conv'd gib_list stuff to feh_node stuff.

*/

#ifdef HAVE_LIBEXIF
#include <libexif/exif-data.h>
#endif

#include "feh.h"
#include "filelist.h"
#include "feh_ll.h"
#include "options.h"


extern int errno;

LLMD * init_LLMD( void ){
    /* Creates the linkedList container (LLMD), returning ptr to same.
     * sets up the 0th rn (root_node) for it.
     * Do I need to clear out all the flags?
     */

  LLMD *md = (LLMD *) emalloc(sizeof( LLMD ));

  /* all new feh_nodes have next and prev linked to itself
   * so there are NO NULL ptrs to deal with.
   */
  md->rn = feh_ll_new();      /* sets md->nd.cnt=0 */
  md->cn = md->rn;            /* safely signals it is empty */

  return md;
}


feh_file *feh_file_new(char *filename)
{
	feh_file *newfile;
	char *s;

	newfile = (feh_file *) emalloc(sizeof(feh_file));
	newfile->caption = NULL;
	newfile->filename = estrdup(filename);
	s = strrchr(filename, '/');
	if (s)
		newfile->name = estrdup(s + 1);
	else
		newfile->name = estrdup(filename);
	newfile->info = NULL;
#ifdef HAVE_LIBEXIF
	newfile->ed = NULL;
#endif
	return(newfile);

}     /* end of feh_file_new() */

void feh_file_free(feh_file * file)
{
	if (!file)
		return;
	if (file->filename)
		free(file->filename);
	if (file->name)
		free(file->name);
	if (file->caption)
		free(file->caption);
	if (file->info)
		feh_file_info_free(file->info);
#ifdef HAVE_LIBEXIF
	if (file->ed)
		exif_data_unref(file->ed);
#endif
	free(file);
	return;

}     /* end of feh_file_free() */

void feh_file_free_md( LLMD *md ) {
    /* This is a combination of feh_file_free() and   feh_ll_free()
     * that loops thru the entire list once, freeing everything.
     * Returns with a completely purged list and an empty rn (root_node)
     * Start at the back of the list and free toward the start.
     */
  feh_node *l;

	for (l = md->rn->prev ; l != md->rn ; l = l->prev ) {
		feh_file_free(l->data);
    free(l);
	}

  /* all done so clear out cnts and flags ... */
  md->rn->nd.cnt = 0;
  md->rn->nd.dirty = 0;
  md->rn->nd.tagged = 0;

  /* and make everything point to the rn */
  md->rn->next = md->rn;
  md->rn->prev = md->rn;
  md->cn = md->rn;

  return;

}     /*end of feh_file_free_md() */

feh_file_info *feh_file_info_new(void)
{
	feh_file_info *info;


	info = (feh_file_info *) emalloc(sizeof(feh_file_info));

	info->width = 0;
	info->height = 0;
	info->size = 0;
	info->pixels = 0;
	info->has_alpha = 0;
	info->format = NULL;
	info->extension = NULL;

	return(info);
}

void feh_file_info_free(feh_file_info * info)
{
	if (!info)
		return;
	if (info->format)
		free(info->format);
	if (info->extension)
		free(info->extension);
	free(info);
	return;
}

void feh_file_rm_and_free( LLMD *md )
{
	unlink(FEH_FILE(md->cn->data)->filename);
	feh_file_remove_from_list( md );
  return;
}

void feh_file_remove_from_list( LLMD *md )
{
	feh_file_free(FEH_FILE(md->cn->data));
  feh_ll_remove( md );
	return;
}

int file_selector_all(const struct dirent *unused __attribute__((unused)))
{
  return 1;
}

static void feh_print_stat_error(char *path)
{
	if (opt.quiet)
		return;

	switch (errno) {
	case ENOENT:
	case ENOTDIR:
		weprintf("%s does not exist - skipping", path);
		break;
	case ELOOP:
		weprintf("%s - too many levels of symbolic links - skipping", path);
		break;
	case EACCES:
		weprintf("you don't have permission to open %s - skipping", path);
		break;
	case EOVERFLOW:
		weprintf("Cannot open %s - EOVERFLOW.\n"
			"Recompile with stat64=1 to fix this", path);
		break;
	default:
		weprintf("couldn't open %s", path);
		break;
	}
}     /* end of file_selector_all() */


/* Recursive */
void add_file_to_filelist_recursively(char *origpath, unsigned char level)
{
	struct stat st;
	char *path;

	if (!origpath)
		return;

	path = estrdup(origpath);
	D(("file is %s\n", path));

	if (level == FILELIST_FIRST) {
		/* First time through, sort out pathname */
		int len = 0;

		len = strlen(path);
		if (path[len - 1] == '/')
			path[len - 1] = '\0';

		if ((!strncmp(path, "http://", 7))
				|| (!strncmp(path, "https://", 8))
				|| (!strncmp(path, "ftp://", 6))) {
			/* Its a url */
			D(("Adding url %s to filelist\n", path));
			/* filelist = gib_list_add_front(filelist, feh_file_new(path)); */
      feh_ll_add_end( feh_md, feh_file_new(path));
			/* We'll download it later... */
			free(path);
			return;
		} else if (opt.filelistfile) {
			char *newpath = feh_absolute_path(path);

			free(path);
			path = newpath;
		}
	}

	errno = 0;
	if (stat(path, &st)) {
		feh_print_stat_error(path);
		free(path);
		return;
	}

	if ((S_ISDIR(st.st_mode)) && (level != FILELIST_LAST)) {
		struct dirent **de;
		DIR *dir;
		int cnt, n;

		D(("It is a directory\n"));

		if ((dir = opendir(path)) == NULL) {
			if (!opt.quiet)
				weprintf("couldn't open directory %s:", path);
			free(path);
			return;
		}
		n = scandir(path, &de, file_selector_all, alphasort);
		if (n < 0) {
			switch (errno) {
			case ENOMEM:
				weprintf("Insufficient memory to scan directory %s:", path);
				break;
			default:
				weprintf("Failed to scan directory %s:", path);
			}
		}

		for (cnt = 0; cnt < n; cnt++) {
			if (strcmp(de[cnt]->d_name, ".")
					&& strcmp(de[cnt]->d_name, "..")) {
				char *newfile;

				newfile = estrjoin("", path, "/", de[cnt]->d_name, NULL);

				/* This ensures we go down one level even if not fully recursive
				   - this way "feh some_dir" expands to some_dir's contents */
				if (opt.recursive)
					add_file_to_filelist_recursively(newfile, FILELIST_CONTINUE);
				else
					add_file_to_filelist_recursively(newfile, FILELIST_LAST);

				free(newfile);
			}
			free(de[cnt]);
		}
		free(de);
		closedir(dir);
	} else if (S_ISREG(st.st_mode)) {
		D(("Adding regular file %s to filelist\n", path));
		/* filelist = gib_list_add_front(filelist, feh_file_new(path)); */
		feh_ll_add_end( feh_md, feh_file_new(path));
	}
	free(path);
	return;
}

void feh_file_info_preload( LLMD *md ) {
    /* Run thru the whole list and toss out any that cannot be loaded. */
	feh_node *l;
	feh_file *file = NULL;

	md->rn->nd.tagged=0;            /* to know if any were tagged */

	for ( l = md->rn->next ; l != md->rn ; l = l->next) {
		file = FEH_FILE(l->data);
		D(("file %p, file->next %p, file->name %s\n", l, l->next, file->name));
		if (feh_file_info_load(file, NULL)) {
			D(("Failed to load file %p\n", file));
			l->nd.tagged=1;             /* this one is tagged for remove */
			md->rn->nd.tagged=1;        /* to know if any were tagged */
			if (opt.verbose)
				feh_display_status('x');
		} else if (opt.verbose)
			feh_display_status('.');
	}
	if (opt.verbose)
		feh_display_status(0);

	if ( md->rn->nd.tagged ) {
		for ( l = md->rn->next ; l != md->rn ; l = l->next)
        if ( l->nd.tagged ) {
            md->cn = l;
            /* does NOT free data cause failed load never made a data member */
            feh_ll_remove( md );
        }
    /* turn off the tagging flag and recnt everything */
    md->rn->nd.tagged=0;
    feh_ll_recnt( md );
	}

	return;

}   /* end of feh_file_info_preload() */

int feh_file_info_load(feh_file * file, Imlib_Image im)
{
	struct stat st;
	int need_free = 1;
	Imlib_Image im1;

	D(("im is %p\n", im));

	if (im)
		need_free = 0;

	errno = 0;
	if (stat(file->filename, &st)) {
		feh_print_stat_error(file->filename);
		return(1);
	}

	if (im)
		im1 = im;
	else if (!feh_load_image(&im1, file) || !im1)
		return(1);

	file->info            = feh_file_info_new();
	file->info->width     = gib_imlib_image_get_width(im1);
	file->info->height    = gib_imlib_image_get_height(im1);
	file->info->has_alpha = gib_imlib_image_has_alpha(im1);
	file->info->pixels    = file->info->width * file->info->height;
	file->info->format    = estrdup(gib_imlib_image_format(im1));
	file->info->size      = st.st_size;

	if (need_free)
		gib_imlib_free_image_and_decache(im1);

	return(0);

}     /* end of feh_file_info_load() */

/*      This works, but yuck to read...
int feh_cmp_filename(const void *node1, const void *node2)
{
	return(strcmp( ((feh_file*)(**(feh_node**)node1).data)->filename,
                 ((feh_file*)(**(feh_node**)node2).data)->filename ) );
}
*/
#define FEH_NODE_DATA( n )    ((feh_file*)(**(feh_node**)n ).data)
int feh_cmp_filename(const void *node1, const void *node2)
{
	return(strcmp( FEH_NODE_DATA( node1 )->filename,
                 FEH_NODE_DATA( node2 )->filename ) );
}

int feh_cmp_name(const void *node1, const void *node2)
{
	return(strcmp( FEH_NODE_DATA( node1 )->name,
                 FEH_NODE_DATA( node2 )->name ) );
}

int feh_cmp_width(const void *node1, const void *node2)
{
	return( FEH_NODE_DATA(node1)->info->width - FEH_NODE_DATA(node2)->info->width );
}

int feh_cmp_height(const void *file1, const void *file2)
{
	return( FEH_NODE_DATA(file1)->info->height - FEH_NODE_DATA(file2)->info->height );
}

int feh_cmp_pixels(const void *node1, const void *node2)
{
	return( FEH_NODE_DATA(node1)->info->pixels - FEH_NODE_DATA(node2)->info->pixels );
}

int feh_cmp_size(const void *node1, const void *node2)
{
	return( FEH_NODE_DATA(node1)->info->size - FEH_NODE_DATA(node2)->info->size );
}

int feh_cmp_format(const void *node1, const void *node2)
{
	return(strcmp(FEH_NODE_DATA(node1)->info->format, FEH_NODE_DATA(node2)->info->format));
}

void feh_prepare_filelist( LLMD *md ) {
    /* load the initial list of pics, with or without a sort order.
     */
	if (opt.list || opt.customlist || (opt.sort > SORT_FILENAME)
			|| opt.preload) {
		/* For these sort options, we have to preload images */
		feh_file_info_preload( md );
    if ( feh_md->rn->nd.cnt == 0 )
			show_mini_usage();
	}

	D(("sort mode requested is: %d\n", opt.sort));
	switch (opt.sort) {
	case SORT_NONE:
		if (opt.randomize) {
			/* Randomize the filename order */
			feh_ll_randomize( md );
		  opt.reverse = 0;                  /* regardless.  No sense in reversing a random */
		}
		break;
	case SORT_NAME:
		feh_ll_qsort( md , feh_cmp_name);
		break;
	case SORT_FILENAME:
		feh_ll_qsort( md , feh_cmp_filename);
		break;
	case SORT_WIDTH:
		feh_ll_qsort( md , feh_cmp_width);
		break;
	case SORT_HEIGHT:
		feh_ll_qsort( md , feh_cmp_height);
		break;
	case SORT_PIXELS:
		feh_ll_qsort( md , feh_cmp_pixels);
		break;
	case SORT_SIZE:
		feh_ll_qsort( md , feh_cmp_size);
		break;
	case SORT_FORMAT:
		feh_ll_qsort( md , feh_cmp_format);
		break;
	default:
		break;
	}

	/* if randomize() was done, then the rev flag is OFF */
	if ( opt.reverse ) {
		D(("Reversing filelist as requested\n"));
    feh_ll_reverse( md );
	}

	return;
}

int feh_write_filelist( LLMD * md, char *filename) {
    /* write out the whole linked list to the user supplied filename
     */

	FILE *fp;
	feh_node *l;

	if ( (md->rn->next == md->rn) || !filename || !strcmp(filename, "/dev/stdin"))
		return(0);

	errno = 0;
	if ((fp = fopen(filename, "w")) == NULL) {
		weprintf("can't write filelist %s:", filename);
		return(0);
	}

	for ( l = md->rn->next ; l != md->rn ; l = l->next)
		fprintf(fp, "%s\n", (FEH_FILE(l->data)->filename));

	fclose(fp);

	return(1);
}     /* end of feh_write_filelist() */

void feh_read_filelist( LLMD *md , char *filename) {
    /* this is a change to the old code.  This one just tacks on any new list
     * found in filename (if any) onto the tail of the existing list (md->rn)
     */

	FILE *fp;
	char s[1024], s1[1024];
	/* Imlib_Image tmp_im;      removed May 2012 */
	struct stat st;

	if (!filename)
		return;

	/*
	 * feh_load_image will fail horribly if filename is not seekable
	 */
  /* && feh_load_image_char(&tmp_im, filename)) ;;; code replaced May 2012 */
	if (!stat(filename, &st)
        && S_ISREG(st.st_mode)
        && ( is_filelist( filename ) != 0 ) ) {

      weprintf("Filelist file %s is NOT a list!  Refusing to use it.\n"
			"Did you mix up -f and -F?", filename);
		opt.filelistfile = NULL;
		return;
	}

	errno = 0;
	if ((fp = fopen(filename, "r")) == NULL) {
		/* return quietly, as it's okay to specify a filelist file that doesn't
		   exist. In that case we create it on exit. */
		return;
	}

	for (; fgets(s, sizeof(s), fp);) {
		D(("Got line '%s'\n", s));
		s1[0] = '\0';
		sscanf(s, "%[^\n]", (char *) &s1);
		if (!(*s1) || (*s1 == '\n'))
			continue;
		D(("Got filename %s from filelist file\n", s1));
		/* Add it to the tail end of the existing list */
		feh_ll_add_end( md , feh_file_new(s1));
	}
	fclose(fp);

  /* whether we got a new list or not, recnt() */
  feh_ll_recnt( md );

	return;

}     /* end of feh_read_filelist() */

char *feh_absolute_path(char *path)
{
	char cwd[PATH_MAX];
	char fullpath[PATH_MAX];
	char temp[PATH_MAX];
	char *ret;

	if (!path)
		return(NULL);
	if (path[0] == '/')
		return(estrdup(path));
	/* This path is not relative. We're gonna convert it, so that a
	   filelist file can be saved anywhere and feh will still find the
	   images */
	D(("Need to convert %s to an absolute form\n", path));
	/* I SHOULD be able to just use a simple realpath() here, but dumb *
	   old Solaris's realpath doesn't return an absolute path if the
	   path you give it is relative. Linux and BSD get this right... */
	if ( getcwd(cwd, sizeof(cwd)) )
      snprintf(temp, sizeof(temp), "%s/%s", cwd, path);
	if (realpath(temp, fullpath) != NULL) {
		ret = estrdup(fullpath);
	} else {
		ret = estrdup(temp);
	}
	D(("Converted path to %s\n", ret));
	return(ret);
}

void feh_save_filelist( void )
{
	char *tmpname;

	tmpname = feh_unique_filename("", "filelist");

	if (opt.verbose)
		printf("saving filelist to filename '%s'\n", tmpname);

	feh_write_filelist( feh_md , tmpname);
	free(tmpname);
	return;
}

/*
 * ****************************************************************
 * May 5, 2012 HRABAK combined list.c into this filelist.c module *
 ******************************************************************
 */


void init_list_mode(void)
{
	feh_node *l;
	feh_file *file = NULL;
	int j = 0;

	mode = "list";

	if (!opt.customlist)
		fputs("NUM\tFORMAT\tWIDTH\tHEIGHT\tPIXELS\tSIZE\tALPHA\tFILENAME\n",
				stdout);

	for ( l = feh_md->rn->next ; l != feh_md->rn ; l = l->next) {
		file = FEH_FILE(l->data);
		if (opt.customlist)
			printf("%s\n", feh_printf(opt.customlist, file));
		else {
			printf("%d\t%s\t%d\t%d\t%s",
          ++j,
					file->info->format,
          file->info->width,
					file->info->height,
					format_size(file->info->pixels));
			printf("\t%s\t%c\t%s\n",
					format_size(file->info->size),
					file->info->has_alpha ? 'X' : '-',
          file->filename);
		}

		feh_action_run(file, opt.actions[0]);
	}
	exit(0);
}

void real_loadables_mode( int loadable ){
    /* HRABAK killed the init_[un]loadables() and brought that logic here.
     */

  extern char *mode;
	feh_file *file;
	feh_node *l;
	char ret = 0;

	opt.quiet = 1;

  if ( loadable )
      mode = "loadables";
  else
      mode = "unloadables";


	for (l = feh_md->rn->next ; l != feh_md->rn ; l = l->next) {
		Imlib_Image im = NULL;

		file = FEH_FILE(l->data);

		if (feh_load_image(&im, file)) {
			/* loaded ok */
			if (loadable) {
				puts(file->filename);
				feh_action_run(file, opt.actions[0]);
			}
			else ret = 1;

			gib_imlib_free_image_and_decache(im);
		} else {
			/* Oh dear. */
			if (!loadable) {
				puts(file->filename);
				feh_action_run(file, opt.actions[0]);
			}
			else ret = 1;
		}
	}
	exit(ret);
}


void add_file_to_rm_filelist( LLMD *md, char *file, int delete_flag ) {
    /* HRABAK combined the delete_rm_files() here too.
     * just to encapsulate all rm logic.
     */
  if ( md == NULL )
      return;

  if ( delete_flag == RM_LIST_ADDTO)
      feh_ll_add_end( md, feh_file_new(file));
  else {
      /* shouldn't we free the rm_md list ?  Yes, but it is ONLY called at
      * the end of main() at feh_clean_exit() so it does not matter.
      */
      feh_node *l;

      for ( l = md->rn->next ; l != md->rn ; l = l->next)
          unlink(FEH_FILE(l->data)->filename);
	}

	return;
}
