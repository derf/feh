/* feh_ll.c

Copyright (C) 1999,2000 Tom Gilbert.
Copyright (C) 2012 Chritopher Hrabak   last updt = May 26, 2012 cah.

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
* 1) added a new rn (root_node) to the linked list tying the head to the tail
*    of the list so you always know where the tail is (circular LL now)
* 2) added a single 64bit-field struct to each feh_node called nd (for node_data)
*    nd.cnt is 20bits holding the cnt of this node's relative pos to the full
*    cnt of the whole list (which is stored in rn->nd.cnt).  the remainder of
*    the bits hold picture dimensions and other (user definable) flags , like
*    nd.dirty, nd.flipped, nd.tagged (if this node is TAGGED for moving)
* Any giblib dependancy has been removed.

*/

#include <time.h>
#include <stdlib.h>
#include "feh_ll.h"
#include "utils.h"
#include "debug.h"


/* ***************** *
 * Begin functions   *
 * ***************** */

#if 0         /* nobody uses this (yet?) */
void feh_ll_free( LLMD *md , int free_data ){
    /* starting at the "md->cn" (current_node), free it AND everything below it.
     * the free_data flag (+1)frees data too. 0) leave data alone.)
     * NO!  I don't think this routine CAN free the data.
     * See feh_ll_remove() below.
     */
   feh_node *l, *ll, *last;
   l = md->cn;
   last = md->cn->prev;

   if (!l)
      return;

   while (l != md->rn) {
      ll = l;
      l = l->next;
      if ( free_data )      /* FIXME.  I think this is wrong! */
          free(ll->data);
      free(ll);
   }

  /* then relink the tail end to the rn (root_node) */
  md->rn->prev = last;
  last->next = md->rn;
  md->cn = last;            /* make this the new cn (current_node) */
  /* because we trimmed off the tail, the last->nd.cnt should be the total */
  md->rn->nd.cnt = md->cn->nd.cnt ;

  return;

}     /* end of feh_ll_free() */
#endif      /* #if 0 */


#if 0    /* nobody uses this (yet?) */
int feh_ll_add_front( LLMD *md , void *data) {
    /* add a new node between rn (root_node) and rn->next
     * If "0th" (root_node) is the only one, then its rn->next points to itself.
     * Should i set this new one to the cn (current_node)?
     * Returns 0 if no recnt() is needed, else 1 means call feh_ll_recnt().
     */

  feh_node *l;

  l = feh_ll_new();
  l->prev = md->rn;
  l->data = data;
  l->nd.cnt=1;        /* because it is at the head it is #1 in the list */

  if ( md->rn->next == md->rn  ) {
      /* root_node is the only one */
      l->next = md->rn;
      md->rn->next = l;
      md->rn->prev = l;
      md->rn->nd.dirty=0;
      md->rn->nd.cnt=1;             /* only one in the list */

  } else {
      /* already one at the head, so insert this new one between rn and rn->next */
      md->rn->next->prev = l;
      l->next = md->rn->next->next;
      md->rn->next = l;
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

   feh_node *l;

   l = (feh_node *) emalloc(sizeof(feh_node));
   l->nd.cnt = 0;
   l->data = NULL;
   l->next = l;
   l->prev = l;
   return (l);

}   /* end of feh_ll_new() */

void feh_ll_remove( LLMD *md ) {
    /*  unlink the md->cn (current_node) from the list and free it.
     *  But CANNOT free the data part.  The caller has to do that.
     */
  feh_node *l = md->cn;

  if ( md->cn == md->rn )
     return ;

  l->prev->next = l->next;
  l->next->prev = l->prev;

  /* reassign the cn (current_node) to the l->prev, unless it is the first */
  if ( l->prev == md->rn )
      md->cn = md->rn->next;
  else
      md->cn = l->prev;

  md->rn->nd.cnt--;
  md->rn->nd.dirty=1;

  /* NEW (wrong) logic.  I thought the old feh had a leak cause it
   * did not free the data too.  But it cannot cause only the user
   * knows what the structure of that data.  So ignore this try!
  if ( l->data )
      free(l->data);
    */

  free(l);

  return;

}   /* end of feh_ll_remove() */


int feh_ll_add_end( LLMD *md , void *data) {
    /* add a new node between rn (root_node) and rn->prev
     * Does NOT change the dirty flag but still returns it.
     * Returns 0 if no recnt() is needed, else 1 means call feh_ll_recnt().
     */

  feh_node *l;

  l = feh_ll_new();
  l->next = md->rn;
  l->data = data;
  md->rn->nd.cnt++;
  l->nd.cnt=md->rn->nd.cnt;             /* last one in the list */

  if ( md->rn->prev == md->rn ) {
      /* root_node is the only one */
      l->prev = md->rn;
      md->rn->prev = l;
      md->rn->next = l;
      md->rn->nd.cnt = l->nd.cnt=1;     /* only one in the list */
      md->rn->nd.dirty=0;
  } else {
      /* already one at the end, so insert this new one between it and rn */
      l->prev = md->rn->prev;
      md->rn->prev->next = l;
      md->rn->prev = l;
      /* md->rn->nd.dirty=0;   just leave the dirty flag as is, whatever it is */
  }

  return (md->rn->nd.dirty);      /* tells caller to do feh_ll_recnt() */

}     /* end of feh_ll_add_end() */

void feh_ll_recnt( LLMD *md ){
    /* the rn->dirty flag is probably set indicating we need to exhaustively
     * loop thru the whole list and cnt everybody again.  Once this is done,
     * each feh_node->nd.cnt will hold its relative place cnt in the list
     * and rn->nd.cnt will hold the total cnt for the whole list.
     */

  unsigned i  = 0;
  feh_node *l = md->rn;

  /* start at root_node and loop till we return to root_node */
  while ( (l=l->next) != md->rn )
      l->nd.cnt = ++i;

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
  feh_node *l;
  int delta_rn;                           /* offset relative to rn */
  int delta_cn;

  if ( md->cn->nd.cnt == pos )
      return;             /* nothing to do.  already there */

  if ( md->rn->nd.cnt <= pos ) {
      md->cn = md->rn;    /* jump as far as we can */
      return;
  }

  /* how far is the jump from where we are? */
  delta_cn = pos - md->cn->nd.cnt ;       /* gives a +/- offset */

  if ( delta_cn > 0 ){
      /* its ahead of cn, so is it closer to rn? */
      delta_rn = md->rn->nd.cnt - pos ;
      if ( delta_cn <  delta_rn ) {
          /* its in front of cn */
          l = md->cn->next;
          while (l->nd.cnt != pos )
              l = l->next;                /* ... going forwards */
      } else {
          /* closer to start from rn */
          l = md->rn->prev;
          while (l->nd.cnt != pos )
              l = l->prev;                /* ... going backwards */
      }
  } else {
      /* its behind cn */
      delta_cn*=-1;                        /* make the delta abs() */
      if ( (unsigned) delta_cn <  pos ) {
          l = md->cn->prev;
          while (l->nd.cnt != pos )
              l = l->prev;                /* ... going backwards */
      } else {
          /* closer to start from rn */
          l = md->rn->next;
          while (l->nd.cnt != pos )
              l = l->next;                /* ... going forward */
      }
  }

  /* l is now the feh_node we want, so make that the new cn (current_node)  */
  md->cn = l;

  return;

}   /* end of feh_ll_nth() */

void feh_ll_reverse( LLMD *md ) {
    /* reversing a list is just swapping l->next and l->prev
     */

   feh_node *l;

   md->cn = md->rn;           /* start with the root */
   if ( md->cn->next == md->rn )
      return;                 /* catches zero and one node */

    do {
      l = md->cn->next;       /* just a tmp save on one of them */
      md->cn->next = md->cn->prev;
      md->cn->prev = l ;
      md->cn = l;
   } while ( md->cn != md->rn );


   /* recnt() in the new order.  I could renumber during rev process :/ */
   feh_ll_recnt( md );

   return ;

}     /* end of feh_ll_reverse() */


void feh_ll_qsort( LLMD *md , feh_compare_fn cmp) {
    /* make a generic frontend for sorting feh_nodes using qsort.
     * This can be used to randomize the list given the appropriate cmp func.
     */

   int len, i;
   feh_node **farray, *f, *t;

  /* probably a good time to be sure our rn->nd.cnt is correct */
  feh_ll_recnt( md );

  if (md->rn->nd.cnt < 2 )
      return ;        /* nothing to do, cause only one (or none) in the list */

  len = md->rn->nd.cnt;
  farray = (feh_node **) emalloc(sizeof(feh_node *) * len);

  f = md->rn->next;
  for ( i = 0; f != md->rn ; f = f->next, i++)
      farray[i] = f;

  qsort( farray, len , sizeof(feh_node *),  cmp );

  /* farray is now sorted in whatever order you chose with the cmp func
   * so time to put the full linked list back together again in that order
   */

  len--;
  t = farray[0];
  t->prev = md->rn;
  md->rn->next = t;
  t->next = f = farray[1];
  for (i = 1 ; i < len ; i++ ) {
     f->prev = farray[i - 1];
     f->next = farray[i + 1];
     f = f->next;
  }

  /* all nodes linked again so just close the tail back to the root_node */
  f->prev = farray[len - 1];
  f->next = md->rn;
  md->rn->prev = farray[len];
  free(farray);

  /* recount the rearranged list, and set the current one to the first */
  feh_ll_recnt( md );
  md->cn = md->rn->next;

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

