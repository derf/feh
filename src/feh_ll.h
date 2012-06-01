/* feh_ll.h

Copyright (C) 1999,2000 Tom Gilbert.
Copyright (C) 2012 Chritopher Hrabak   last updt=   May 28, 2012 cah.

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

As of Apr 2012, HRABAK hacked the orig gib_list.c code to support two major
changes in the way feh uses its linked list of pics.
* 1) added a new root_node to the linked list tieing the head to the tail
*    of the list so you always know where the tail is (circular LL now)
* 2) added a single 64bit struct to each feh_node (called nd for node_data)
*    which is a bit-field.  nd.cnt is 20bits holding the cnt of this node
*    relative to the full cnt of the whole list (which is stored in
*    root_node->nd.cnt).  the remainder of the bits are for flags tied to
*    each node (like if this node TAGGED for moving)
* Any giblib dependancy has been removed.   Hrabak named all the new
* replacement functions feh_ll_???() rather than gib_list_???() or
* even feh_list_???() to show that these are all LinkList functions
* that are very diff than the original gib_list-named functions.

*/

#ifndef FEH_LL_H
#define FEH_LL_H

#include "structs.h"
#include <stdarg.h>

typedef int (feh_compare_fn) (const void *data1, const void *data2);

/*
******************************************************************************
* define a suite of macros to make access to the LLMD members more readable
* these have been moved to feh.h ....

#define FEH_LL_ROOT(md)               (( md->rn ))
#define FEH_LL_LEN(md)                (( md->rn->nd.cnt ))
#define FEH_LL_FIRST(md)              (( md->rn->next ))
#define FEH_LL_LAST(md)               (( md->rn->prev ))
#define FEH_LL_DIRTY(md)              (( md->rn->nd.dirty ))
#define FEH_LL_TAGGED(md)             (( md->rn->nd.tagged ))

#define FEH_LL_CUR(md)                (( md->cn ))
#define FEH_LL_CUR_NTH(md)            (( md->cn->nd.cnt ))
#define FEH_LL_CUR_TAGGED(md)         (( md->cn->nd.tagged ))
#define FEH_LL_CUR_NEXT(md)           (( md->cn->next ))
#define FEH_LL_CUR_PREV(md)           (( md->cn->prev ))
#define DECREMENT_CUR_NTH(md)         (( md->cn->nd.cnt-- ))
#define INCREMENT_CUR_NTH(md)         (( md->cn->nd.cnt++ ))
*****************************************************************************
*/

#ifdef __cplusplus
extern "C"
{
#endif


feh_node *feh_ll_new(void);
void feh_ll_free( LLMD *md , int free_data );
int feh_ll_add_front( LLMD *md , void *data);
int feh_ll_add_end( LLMD *md , void *data);
void feh_ll_recnt( LLMD *md );
void feh_ll_nth( LLMD * md, unsigned pos );
void feh_ll_qsort( LLMD *md , feh_compare_fn cmp);
void feh_ll_randomize( LLMD *md );
void feh_ll_remove( LLMD *md );
void feh_ll_reverse( LLMD *md );
int feh_cmp_random( const void *node1, const void *node2);

#ifdef __cplusplus
}
#endif


#endif   /* of #ifndef FEH_LL_H */
