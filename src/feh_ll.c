/* feh_ll.c

Copyright (C) 1999,2000 Tom Gilbert.
Copyright (C) 2010-2011 Daniel Friesel.
Copyright (C) 2012 Chritopher Hrabak   last updt = Mar, 25, 2013 cah.

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

* See feh_ll.h for HRABAK notes on hacking the orig gib_list.c code to
* remove any giblib dependancy.
*
*/

#ifdef HAVE_LIBEXIF
	#include <libexif/exif-data.h>
#endif

#include <time.h>
#include <stdlib.h>
#include "feh.h"
#include "feh_ll.h"
#include "options.h"
#include "debug.h"

extern int errno;


/* ***************** *
 * Begin functions   *
 * ***************** */


#if 0    /* nobody uses this (yet?) */
int feh_ll_add_front( LLMD *md , void *data) {
		/* add a new node between rn (root_node) and rn->next
		* If "0th" (root_node) is the only one, then its rn->next points to itself.
		* Should i set this new one to the cn (current_node)?
		* Returns 0 if no recnt() is needed, else 1 means call feh_ll_recnt().
		*/

	feh_node *node;

	node = feh_ll_new();
	node->prev = md->rn;
	node->data = data;
	node->nd.cnt=1;        /* because it is at the head it is #1 in the list */

	if ( md->rn->next == md->rn  ) {
			/* root_node is the only one */
			node->next = md->rn;
			md->rn->next = node;
			md->rn->prev = node;
			md->rn->nd.dirty=0;
			md->rn->nd.cnt=1;             /* only one in the list */

	} else {
			/* already one at the head, so insert this new one between rn and rn->next */
			md->rn->next->prev = node;
			node->next = md->rn->next->next;
			md->rn->next = node;
			md->rn->nd.cnt++;
			md->rn->nd.dirty=1;
	}

	return (md->rn->nd.dirty);      /* tells caller to do feh_ll_recnt() */

}     /* end of feh_ll_add_front() */

#endif    /*  #if 0 */


feh_node * feh_ll_new(void){
		/* creates a new (unlinked) node for a feh_node-type linkedList.
		* Be sure to call init_LLMD() first time just to make the 0th
		* rn (root_node) anchor.  Afterwards, call once for each new node.
		* All nodes are linked (circularly) to the rn (root_node).
		* Note:  prev & next point to itself so NO future checks need
		* worry about  a NULL ptr.
		*/

	feh_node *node;

	node = (feh_node *) ecalloc(sizeof(feh_node));  /* zeros out ALL members */
	/*node->nd.cnt = 0; */
	/*node->data = NULL;*/
	node->next = node;
	node->prev = node;
	return (node);

}   /* end of feh_ll_new() */

void feh_ll_unlink( LLMD *md, int free_me ) {
		/*  unlink the md->cn (current_node) from the list and free it.
		*  CANNOT free the data part.  The caller better have done
		*  that already.
		*/

	if ( md->cn == md->rn )
		return ;

	md->tn = md->cn;
	md->tn->prev->next = md->tn->next;
	md->tn->next->prev = md->tn->prev;

	/* reassign the cn (current_node) to the ->next, unless it is the last */
	if ( md->tn->next == md->rn )
			md->cn = md->rn->next;
	else
			md->cn = md->tn->next;

	md->rn->nd.cnt--;
	md->rn->nd.dirty=1;
	md->rn->nd.lchange=1;

	if (free_me == FREE_YES )
		free(md->tn);

	return;

}   /* end of feh_ll_unlink() */

void feh_ll_add_end( LLMD *md , void *data) {
		/* add a new node between rn (root_node) and rn->prev, so nd.cnt is
		* incremented and (should always) be correct. So don't set nd.dirty.
		*/

	feh_node *node;

	node = feh_ll_new();
	node->data = data;
	feh_ll_link_at_end( md , node );

}     /* end of feh_ll_add_end() */

void feh_ll_link_at_end( LLMD *md , feh_node *node ) {
		/* No storage allocation - just links this feh_node to the end */

	/* if root_node is the only one, then this is #1 in the list */
	if ( md->rn->prev == md->rn )
		md->rn->nd.dirty=md->rn->nd.cnt=node->nd.cnt=0;       /* just for safety */

	/* even if rn is the ONLY one, insert this new one B4 rn */
	node->prev = md->rn->prev;
	md->rn->prev->next = node;
	md->rn->prev = node;
	node->next = md->rn;
	md->rn->nd.cnt++;
	node->nd.cnt = md->rn->nd.cnt;   /* last one in the list */

}     /* end of feh_ll_link_at_end() */

void feh_ll_recnt( LLMD *md ){
		/* the rn->dirty flag is probably set indicating we need to exhaustively
		* loop thru the whole list and cnt everybody again.  Once this is done,
		* each feh_node->nd.cnt will hold its relative place cnt in the list
		* and rn->nd.cnt will hold the total cnt for the whole list.
		*/

	unsigned short i  = 0;

	/* start at root_node and loop till we return to root_node */
	md->tn = md->rn;
	while ( (md->tn=md->tn->next) != md->rn )
			md->tn->nd.cnt = ++i;

	/* back at the rn, so post the total cnt there , and clear dirty flag */
	md->rn->nd.cnt=i;
	md->rn->nd.dirty=0;

}   /*  end of feh_ll_recnt() */

void feh_ll_nth(LLMD * md, unsigned pos ){
		/* caller wants to jump to an absolute pos(ition) in the list.
		* This routine figures out if it's faster to find that pos starting at the
		* cn (current_node) or starting at the rn (root_node) rather than ALWAYS
		* starting from rn.
		*/
	int delta_rn;               /* offset relative to rn */
	int delta_cn;

	if ( md->cn->nd.cnt == pos )
			return;                 /* nothing to do.  already there */

	if ( md->rn->nd.cnt <= pos ) {
			md->cn = md->rn->prev;  /* jumps to the last one */
			return;
	}

	if ( pos == 0 ) {
			md->cn = md->rn->next;  /* first one             */
			return;
	}

	/* how far is the jump from where we are? */
	delta_cn = pos - md->cn->nd.cnt ;       /* gives a +/- offset */

	if ( delta_cn > 0 ){
			/* its ahead of cn, so is it closer to rn? */
			delta_rn = md->rn->nd.cnt - pos ;
			if ( delta_cn <  delta_rn ) {     /* its in front of cn */
					md->tn = md->cn->next;
					while (md->tn->nd.cnt != pos )
							md->tn = md->tn->next;    /* ... going forwards */
			} else {                          /* closer to start from rn */
					md->tn = md->rn->prev;
					while (md->tn->nd.cnt != pos )
							md->tn = md->tn->prev;    /* ... going backwards */
			}
	} else {
			/* its behind cn */
			delta_cn*=-1;                     /* make the delta abs() */
			if ( (unsigned) delta_cn <  pos ) {
					md->tn = md->cn->prev;
					while (md->tn->nd.cnt != pos )
							md->tn = md->tn->prev;    /* ... going backwards */
			} else {                          /* closer to start from rn */
					md->tn = md->rn->next;
					while (md->tn->nd.cnt != pos )
							md->tn = md->tn->next;    /* ... going forward */
			}
	}

	md->cn = md->tn;                      /* tn is node we want  */
	/*printf("jumped to %dof%d pos==%d\n", md->tn->nd.cnt,md->rn->nd.cnt,pos);*/
	return;

}   /* end of feh_ll_nth() */

void feh_ll_reverse( LLMD *md ) {
		/* reversing a list is just swapping node->next and node->prev */

	feh_node *node;

	md->tn = md->rn;           /* start with the root */
	if ( md->tn->next == md->rn )
		return;                 /* catches zero and one node */

	do {
		node = md->tn->next;       /* just a tmp save on one of them */
		md->tn->next = md->tn->prev;
		md->tn->prev = node;
		md->tn = node;
	} while ( md->tn != md->rn );


	/* recnt() in the new order.  I could renumber during rev process :/ */
	feh_ll_recnt( md );

	return ;

}     /* end of feh_ll_reverse() */


void feh_ll_qsort( LLMD *md , feh_compare_fn cmp) {
		/* make a generic frontend for sorting feh_nodes using qsort.
		* This can be used to randomize the list given the appropriate cmp func.
		*/

	int len, i;
	feh_node **node_array;

	/* probably a good time to be sure our rn->nd.cnt is correct */
	feh_ll_recnt( md );

	if (md->rn->nd.cnt < 2 )
			return ;        /* nothing to do, cause only one (or none) in the list */

	len = md->rn->nd.cnt;
	node_array = (feh_node **) emalloc(sizeof(feh_node *) * len);

	md->tn = md->rn->next;
	if ( cmp == feh_cmp_size ){
		for ( i = 0; md->tn != md->rn ; md->tn = md->tn->next, i++){
				if ( NODE_DATA(md->tn)->info == NULL )
						feh_ll_load_data_info(NODE_DATA(md->tn), NULL );
				node_array[i] = md->tn;
		}
	} else {
		for ( i = 0; md->tn != md->rn ; md->tn = md->tn->next, i++)
				node_array[i] = md->tn;
	}

	qsort( node_array, len , sizeof(feh_node *),  cmp );

	/* node_array is now sorted in whatever order you chose with the cmp func
	* so time to put the full linked list back together again in that order
	*/

	len--;
	md->tn = node_array[0];
	md->tn->prev = md->rn;
	md->rn->next = md->tn;
	md->tn->next = node_array[1];
	md->tn = node_array[1];
	for (i = 1 ; i < len ; i++ ) {
		md->tn->prev = node_array[i - 1];
		md->tn->next = node_array[i + 1];
		md->tn = md->tn->next;
	}

	/* all nodes linked again so just close the tail back to the root_node */
	md->tn->prev = node_array[len - 1];
	md->tn->next = md->rn;
	md->rn->prev = node_array[len];
	free(node_array);

	/* recount the rearranged list.  Setting tn to the first allows user
	 *to return to the node (cn) he was on B4 the sort */
	feh_ll_recnt( md );
	md->tn = md->rn->next;
	/*md->cn = md->rn->next;*/

	LL_CHANGED(md) = 1;         /* to write the list out at exit */

	return ;

}     /* end of feh_ll_qsort() */

/* declare a global variable to be used as a mask for feh_cmp_random() .
 * Set it here, at the end of this module so NO other func() will see it.
 */
  unsigned mask;

void feh_ll_randomize( LLMD *md ){
		/* to ramdomize the list, just choose a random node, and make that into
		* a mask to apply to a pair of node addresses during a qsort() compare.
		*/

	/* pick a random node between 1 and rn.cnt */
	feh_ll_nth( md, ( (unsigned) ( time(NULL) % md->rn->nd.cnt) +1 ));

	/* use that feh_node address (md->cn) to make a two's-compliment mask */
	mask = ~(unsigned) md->cn ;             /* mask is a global variable */

	/* the qsort() cmp func will EXCL-OR that mask against node1 and node2 for the compare */
	feh_ll_qsort( md , feh_cmp_random);

	/* qsort() took care to reset the start to whatever node is first now */

	return ;

}   /* end of feh_ll_randomize() */

#define FEH_NODE_NUM( n )       ((unsigned )(*(feh_node**)n))

int feh_cmp_random( const void *node1, const void *node2) {
		/* rather that use the rand() logic to randomize the pics (who cares how
		* random they really are?), just make a bogus qsort() cmp func that
		* succeeds in messing up the order, like exclusive OR'ing it.
		* Note:  mask is a global var (see above) with a very short scope.
		*/

	return( ( FEH_NODE_NUM( node1 ) ^ mask ) - ( FEH_NODE_NUM( node2 ) ^ mask ) );
}

/* ************************************************************************
* Jun 9, 2013  HRABAK combined the old filelist.c module into this feh_ll.c
* module and renamed all the old feh_file_<named> functions to feh_ll_<named>
* functions.
*/

LLMD * init_LLMD( void ){
		/* Creates the linkedList container (LLMD), returning ptr to same.
		* sets up the 0th rn (root_node) for it.
		* calloc() should take care of clearing out all the flags.
		*/

	LLMD *md = (LLMD *) ecalloc(sizeof( LLMD ));

	/* all new feh_nodes have next and prev linked to itself
	* so there are NO NULL ptrs to deal with.
	*/
	md->rn = feh_ll_new();      /* sets md->nd.cnt=0 */
	md->cn = md->rn;            /* safely signals it is empty */

	return md;
}


feh_data *feh_ll_new_data(char *filename)
{
	feh_data *newdata;
	char *s;

	newdata = (feh_data *) emalloc(sizeof(feh_data));
	newdata->caption = NULL;
	newdata->filename = estrdup(filename);

	if ( (s = strrchr(newdata->filename, '/')) )
		newdata->name = (s + 1);
	else
		newdata->name = filename;

	if ( (s = strrchr(newdata->name, '.')) )
		newdata->ext = s ;          /* overwrite with '\0' when --dno-ext  */
	else
		newdata->ext = NULL;


	newdata->info = NULL;
#ifdef HAVE_LIBEXIF
	newdata->ed = NULL;
#endif
	return(newdata);

}     /* end of feh_ll_new_data() */

void feh_ll_free_data(feh_data * data)
{
	if (!data)
		return;
	if (data->filename)
		free(data->filename);     /* name is part of filename */
	if ( !(data->caption == fgv.no_cap
			|| data->caption == NULL ) )
		free(data->caption);
	if (data->info)
		feh_ll_free_data_info(data->info);
#ifdef HAVE_LIBEXIF
	if (data->ed)
		exif_data_unref(data->ed);
#endif
	free(data);
	return;

}     /* end of feh_ll_free_data() */

void feh_ll_free_md( LLMD *md ) {
		/* This loops thru the entire list once, freeing everything.
		* Returns with a completely purged list and an empty rn (root_node)
		* Start at the back of the list and free toward the start.
		*/

	for (md->cn = md->rn->prev ; md->cn != md->rn ; md->cn = md->cn->prev ) {
		feh_ll_free_data(NODE_DATA(md->cn) );
    free(md->cn);
	}

	/* all done so clear out cnts and flags ... */
	md->rn->nd.cnt = 0;
	md->rn->nd.dirty = 0;
	md->rn->nd.lchange = 1;

	/* and make everything point to the rn */
	md->rn->next = md->rn->prev = md->cn = md->rn;

	return;

}     /*end of feh_ll_free_md() */


void feh_ll_free_data_info(feh_data_info * info)
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


int file_selector_all(const struct dirent *unused __attribute__((unused)))
{
	return 1;
}

static void feh_print_stat_error(char *path)
{
	if (opt.flg.quiet)
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
		weprintf("%s%s%s - EOVERFLOW.\n"
			"Recompile with stat64=1 to fix this", ERR_CANNOT, ERR_OPEN, path);
		break;
	default:
		weprintf("%s%s%s", ERR_CANNOT, ERR_OPEN, path);
		break;
	}
}     /* end of feh_print_stat_error() */

/* Recursive */
void add_file_to_filelist_recursively(char *origpath, unsigned char level)
{
	static char *newfile;
	struct stat st;
	char *path;

	if (!origpath)
		return;

	path = estrdup(origpath);
	D(("file is %s\n", path));

	if (level == FILELIST_FIRST) {
		/* First time through, sort out pathname */
		int len = 0;
		newfile = mobs(0);   /* mobs(0) is only shared with feh_printf() */

		len = strlen(path);
		if (path[len - 1] == '/')
			path[len - 1] = '\0';

		if ((!strncmp(path, "http://", 7))
				|| (!strncmp(path, "https://", 8))
				|| (!strncmp(path, "ftp://", 6))) {
			/* Its a url */
			D(("Adding url %s to filelist\n", path));
      feh_ll_add_end( feh_md, feh_ll_new_data(path));
			/* We'll download it later... */
			free(path);
			return;
		} else if (opt.filelistfile) {
			char *newpath = estrdup( feh_absolute_path(path) );
			printf("newpath was set.\n");
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
			if (!opt.flg.quiet)
				weprintf("%s%sdirectory %s:", ERR_CANNOT, ERR_OPEN, path);
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
				weprintf("%s to scan directory %s:",ERR_FAILED, path);
			}
		}

		for (cnt = 0; cnt < n; cnt++) {
			if ( !(  (de[cnt]->d_name[0] == '.' ) &&
			        ((de[cnt]->d_name[1] == '\0') ||
			        ((de[cnt]->d_name[1] == '.' ) &&
			         (de[cnt]->d_name[2] == '\0') ))))
			{
 				/* skipping any . or .. dirs */
				newfile[0] = '\0';
				STRCAT_3ITEMS(newfile,path, "/", de[cnt]->d_name);

				/* This ensures we go down one level even if not fully recursive
				   - this way "feh some_dir" expands to some_dir's contents */
				if (opt.flg.recursive)
					add_file_to_filelist_recursively(newfile, FILELIST_CONTINUE);
				else
					add_file_to_filelist_recursively(newfile, FILELIST_LAST);
			}
			free(de[cnt]);
		}
		free(de);
		closedir(dir);
	} else if (S_ISREG(st.st_mode)) {
		D(("Adding regular file %s to filelist\n", path));
		feh_ll_add_end( feh_md, feh_ll_new_data(path));
	}
	feh_md->rn->nd.lchange=1;   /* to trigger saving a filelist to disk  */
	free(path);
	return;
}     /* end of add_file_to_filelist_recursively() */

void feh_ll_preload_data_info( LLMD *md ) {
		/* Run thru the whole list and toss out any that cannot be loaded. */

	feh_node *next_node = md->rn->next;

	while (next_node != md->rn ) {
		md->cn = next_node;
		next_node = next_node->next;
		if (feh_ll_load_data_info(NODE_DATA(md->cn), NULL)) {
			/* Can't use feh_move_node_to_remove_list() cause
			 * it recnt()'s each time after relinking md->cn */
			feh_md->tn = feh_md->cn;                /* tmp storage */
			feh_ll_unlink( feh_md, FREE_NO );
			feh_md->tn->nd.delete = 0 ;
			/* just relink from feh_md LL to rm_md LL */
			feh_ll_link_at_end( rm_md , feh_md->tn);
			feh_md->rn->nd.lchange = 1;

			if (opt.flg.verbose)
				feh_display_status('x');
		} else if (opt.flg.verbose)
			feh_display_status('.');
	}

	if ( feh_md->rn->nd.lchange )
		feh_ll_recnt( md );

	return;

}   /* end of feh_ll_preload_data_info() */

int feh_ll_load_data_info(feh_data * data, Imlib_Image im)
{
	struct stat st;
	int need_free = 1;
	Imlib_Image im1;

	D(("im is %p\n", im));

	if (im)
		need_free = 0;

	errno = 0;
	if (stat(data->filename, &st)) {
		feh_print_stat_error(data->filename);
		return(1);
	}

	if (im)
		im1 = im;
	else if (!feh_load_image(&im1, data) || !im1)
		return(1);

	/*data->info            = feh_file_info_new();*/
	data->info            = (feh_data_info *) emalloc(sizeof(feh_data_info));
	data->info->width     = gib_imlib_image_get_width(im1);
	data->info->height    = gib_imlib_image_get_height(im1);
	data->info->has_alpha = gib_imlib_image_has_alpha(im1);
	data->info->pixels    = data->info->width * data->info->height;
	data->info->format    = estrdup(gib_imlib_image_format(im1));
	data->info->size      = st.st_size;
	data->info->mtime     = st.st_mtime;            /* added jan 7, 2013 */

	if (need_free)
		gib_imlib_free_image_and_decache(im1);

	return(0);

}     /* end of feh_ll_load_data_info() */

/*      This works, but yuck to read...
int feh_cmp_filename(const void *node1, const void *node2)
{
	return(strcmp( ((feh_data*)(**(feh_node**)node1).data)->filename,
                 ((feh_data*)(**(feh_node**)node2).data)->filename ) );
}
*/
#define FEH_NODE_DATA( n )    ((feh_data*)(**(feh_node**)n ).data)
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

int feh_prepare_filelist( LLMD *md ) {
		/* load the initial list of pics, with or without a sort order.
		*/

	if ( (opt.flg.mode == MODE_LIST)
	   || opt.customlist
	   ||(opt.sort > SORT_FILENAME)
	   || opt.flg.preload) {
		/* For these sort options, we have to preload images */
		feh_ll_preload_data_info( md );
	}

	if ( md->rn->nd.cnt == 0 )
		return 1;               /* error */

	D(("sort mode requested is: %d\n", opt.sort));
	switch (opt.sort) {
	case SORT_NONE:
		if (opt.flg.randomize) {
			/* Randomize the filename order */
			feh_ll_randomize( md );
		  opt.flg.reverse = 0;                  /* regardless.  No sense in reversing a random */
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
	if ( opt.flg.reverse ) {
		D(("Reversing filelist as requested\n"));
		feh_ll_reverse( md );
	}

	return 0;
}

void feh_write_filelist( LLMD * md, char *list_name, unsigned short int bit_pos ) {
		/* Called up to 13 times to write out all or part of the LL to
		 * the user-supplied list_name.  Once for the whole LL, once
		 * each for the nd.list[0-9] lists if those flags were set and once
		 * each if pix were deleted/removed from the LL.
		 * even if no user actions[0-9] have been defined, just pressing [0-9]
		 * will tag this pic to one of the ten lists nd.list[0-9].  If any of
		 * those have been tagged, write them out now.  Note: this routine names
		 * the lists feh[0-9]list.SSL so they are the same each run.  It is up
		 * to the user to do something with those feh?list.SSL files before the
		 * next run.  feh does nothing with them except write them.
		 */

	FILE *fp;
	unsigned short int *flag;

	if ( list_name == NULL ){
		char name[] = "feh_list.SSL";
		name[3] = bit_pos["0123456789"];    /* feh0list.SSL */
		list_name = name;
	}

	if ( !list_name || !strcmp(list_name, "/dev/stdin"))
		return;

	errno = 0;
	if ((fp = fopen(list_name, "w")) == NULL) {
		weprintf("%swrite filelist %s:", ERR_CANNOT, list_name);
		return;
	}

	for ( md->tn = md->rn->next ; md->tn != md->rn ; md->tn = md->tn->next){
		if ( bit_pos < 10 ){
			flag = (unsigned short int *) &md->tn->nd;
			if ( !(*flag & (1<<bit_pos)) )
				continue;             /* nd.list flag not set */
		} else if ( bit_pos == 11 ) {
			/* this is a remove list request */
			if ( md->tn->nd.delete )
				continue;             /* nd.delete flag IS set */
		} else if ( bit_pos == 12 ) {
			/* this is a deleted list request */
			if ( !md->tn->nd.delete )
				continue;             /* nd.delete flag not set */
		}
		fprintf(fp, "%s\n", NODE_FILENAME(md->tn));
	}

	fclose(fp);

	return;
}     /* end of feh_write_filelist() */

void feh_read_filelist( LLMD *md , char *filelistfile) {
		/* this is a change to the old code.  This one just tacks any new list
		* found in filelistfile (if any) to the tail of the existing list (md->rn)
		*/

#define FRF_SIZE 8            /* FRF stands for FileReadFilelist */
	FILE *fp;
	char *s  = mobs( FRF_SIZE );
	char *tail = NULL;
	struct stat st;

	D(("About to load filelist from file\n"));

	if (!filelistfile)
		return;

	/* feh_load_image will fail horribly if filelistfile is not seekable */
	if (!stat(filelistfile, &st)
			&& S_ISREG(st.st_mode)
			&& ( feh_is_filelist( filelistfile ) != 0 ) ) {

		weprintf("'%s' is NOT a list! Did you mix up -f and -F?",
		         filelistfile);
		opt.filelistfile = NULL;
		return;
	}

	errno = 0;
	if ((fp = fopen(filelistfile, "r")) == NULL) {
		/* return quietly, as it's okay to specify a filelist file that doesn't
		 * exist. In that case we create it on exit. */
		return;
	}

	for (; fgets(s, FRF_SIZE * MOBS_ROW_SIZE, fp);) {
		D(("Got line '%s'\n", s));
		if ( (tail = strrchr( s,'\n')) )  tail[0] = '\0';
		if ( ( s[0] == '\0' ) || ( s[0] == '\n')) continue;
		D(("Got filename %s from filelist file\n", s));
		/* cull the list for non-existent filenames    */
		//~ if (!(stat(s, &st) && S_ISREG(st.st_mode)) ) {
		if ( stat(s, &st)) {
			weprintf("%s - File does not exist", s);
			md->rn->nd.lchange = 1;
			continue;
		}
		/* Add it to the tail end of the existing list */
		feh_ll_add_end( md , feh_ll_new_data(s));
	}
	fclose(fp);

	/* whether we got a new list or not, recnt() */
	feh_ll_recnt( md );

	return;

}     /* end of feh_read_filelist() */


char *feh_absolute_path(char *path) {
		/* If this path is not relative. We're gonna convert it, so that a
		* filelist file can be saved anywhere and feh will still find the
		* images.
		* I SHOULD be able to just use a simple realpath() here, but dumb
		* old Solaris's realpath doesn't return an absolute path if the
		* path you give it is relative. Linux and BSD get this right...
		*/

	char *temp     = mobs(8);     /* SIZE_1024 */
	char *fullpath = mobs(8);
	char *ret;

	if (!path)
		return(NULL);
	if (path[0] == '/')
		return (estrdup(path));

	D(("Need to convert %s to an absolute form\n", path));

	if ( getcwd(temp, SIZE_1024 ) ){
			strcat(temp, "/");
			strcat(temp, path );
	} else
			strcat( temp, path );

	if (realpath(temp, fullpath) == NULL)
		ret = temp;     /* failed */
	else
		ret = fullpath;

	D(("Converted path to %s\n", ret));
	return ret;       /* this is a mobs() so no need to free */

}   /* end of feh_absolute_path() */


/*
 * ****************************************************************
 * May 5, 2012 HRABAK combined list.c into this filelist.c module *
 ******************************************************************
 */


void init_list_mode( LLMD *md )
{
	int j = 0;

	if (!opt.customlist)
		fputs("NUM\tFORMAT\tWIDTH\tHEIGHT\tPIXELS\tSIZE\tALPHA\tFILENAME\n",
				stdout);

	for (md->cn = md->rn->next ;  md->cn != md->rn ; md->cn = md->cn->next) {
		if (opt.customlist)
			printf("%s\n", feh_printf(opt.customlist, NODE_DATA(md->cn)));
		else {
			if (!NODE_DATA(md->cn)->info)
				if ( feh_ll_load_data_info(NODE_DATA(md->cn), NULL) )
					continue;           /* failed load */

			printf("%d\t%s\t%d\t%d\t%s\t%s\t%c\t%s\n",
			       ++j,
			       NODE_DATA(md->cn)->info->format,
			       NODE_DATA(md->cn)->info->width,
			       NODE_DATA(md->cn)->info->height,
			       format_size(NODE_DATA(md->cn)->info->pixels),
			       format_size(NODE_DATA(md->cn)->info->size),
			       NODE_DATA(md->cn)->info->has_alpha ? 'X' : '-',
			       NODE_FILENAME(md->cn) );
		}

		feh_action_run( md->cn , opt.actions[0]);
	}
	exit(0);
}

void real_loadables_mode( LLMD *md ){
		/* HRABAK killed the init_[un]loadables() and brought that logic here.
		*/

	char ret = 0;
	char loadable = 0;

	opt.flg.quiet = 1;

	if ( opt.flg.mode == MODE_LOADABLES )
		loadable = !loadable;

	for (md->cn = md->rn->next ; md->cn != md->rn ; md->cn = md->cn->next) {
		Imlib_Image im = NULL;

		if (feh_load_image(&im, NODE_DATA(md->cn) )) {
			/* loaded ok */
			if (loadable) {
				puts( NODE_FILENAME(md->cn));
				feh_action_run( md->cn, opt.actions[0]);
			}
			else ret = 1;

			gib_imlib_free_image_and_decache(im);
		} else {
			/* Oh dear. */
			if (!loadable) {
				puts( NODE_FILENAME(md->cn));
				feh_action_run( md->cn, opt.actions[0]);
			}
			else ret = 1;
		}
	}
	exit(ret);
}

/* these are not used anymore */

#if 0
feh_data_info *feh_ll_new_data_info(void)
{
	feh_data_info *info;


	info = (feh_data_info *) emalloc(sizeof(feh_data_info));

	info->width  = 0;
	info->height = 0;
	info->size   = 0;
	info->pixels = 0;
	info->has_alpha = 0;
	info->format    = NULL;
	info->extension = NULL;

	return(info);
}
#endif /* #if 0 */

#if 0
void feh_ll_remove_node_from_list( LLMD *md , enum misc_flags delete_flag ){
	if ( delete_flag == DELETE_YES )
		unlink( NODE_FILENAME(md->cn) );

	feh_ll_free_data( NODE_DATA(md->cn) );
	feh_ll_unlink( md, FREE_YES );
	return;
}
#endif   /* #if 0 */

/*  end of the old filelist.c module code */