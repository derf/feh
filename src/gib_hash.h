
/* gib_hash.h

Copyright (C) 1999,2000 Paul Duncan.

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

#ifndef GIB_HASH_H
#define GIB_HASH_H

#include "gib_list.h"

#define GIB_HASH(a) ((gib_hash*)a)
#define GIB_HASH_NODE(a) ((gib_hash_node*)a)

typedef struct __gib_hash      gib_hash;
typedef struct __gib_hash_node gib_hash_node;

struct __gib_hash
{
	gib_hash_node *base;
};

struct __gib_hash_node
{
   gib_list  list;
   char     *key;
};

#ifdef __cplusplus
extern "C"
{
#endif

gib_hash_node *gib_hash_node_new(char *key, void *data);
void           gib_hash_node_free(gib_hash_node *node);
void           gib_hash_node_free_and_data(gib_hash_node *node);

gib_hash *gib_hash_new(void);
void      gib_hash_free(gib_hash *hash);
void      gib_hash_free_and_data(gib_hash *hash);

void      gib_hash_set(gib_hash *hash, char *key, void *data);
void     *gib_hash_get(gib_hash *hash, char *key);

/* unused
void      gib_hash_remove(gib_hash *hash, char *key);

void      gib_hash_foreach(gib_hash *hash, void (*foreach_cb)(gib_hash_node *node, void *data), void *data);
*/

#ifdef __cplusplus
}
#endif

#endif /* GIB_HASH_H */
