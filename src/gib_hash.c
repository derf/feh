/* gib_hash.c

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
#include <strings.h>

#include "gib_hash.h"
#include "utils.h"
#include "debug.h"

gib_hash_node *gib_hash_node_new(char *key, void *data)
{
	gib_hash_node *node = emalloc(sizeof(gib_hash_node));
	node->key = strdup(key);
	GIB_LIST(node)->data = data;
	GIB_LIST(node)->next = NULL;
	GIB_LIST(node)->prev = NULL;
	return node;

}

void           gib_hash_node_free(gib_hash_node *node)
{
	free(node->key);
	free(node);
	return;
}

void           gib_hash_node_free_and_data(gib_hash_node *node)
{
	free(node->list.data);
	gib_hash_node_free(node);
	return;
}

gib_hash *gib_hash_new(void)
{
	gib_hash *hash = emalloc(sizeof(gib_hash));
	hash->base = gib_hash_node_new("__gib_hash_new",NULL);
	return hash;
}

void      gib_hash_free(gib_hash *hash)
{
	/* free hash keys as it's not taken care of by gib_list_free */
	gib_list *i;
	for (i = GIB_LIST(hash->base); i; i = i->next)
		free(GIB_HASH_NODE(i)->key);

	gib_list_free(GIB_LIST(hash->base));
	free(hash);
	return;
}

void      gib_hash_free_and_data(gib_hash *hash)
{
	/* free hash keys as it's not taken care of by gib_list_free */
	gib_list *i;
	for (i = GIB_LIST(hash->base); i; i = i->next)
		free(GIB_HASH_NODE(i)->key);

	gib_list_free_and_data(GIB_LIST(hash->base));
	free(hash);
	return;
}

static unsigned char gib_hash_find_callback(gib_list *list, void *data)
{
	gib_hash_node *node = GIB_HASH_NODE(list);
	char          *key  = (char*) data;

	/* strncasecmp causes similar keys like key1 and key11 clobber each other */
	return !strcasecmp(node->key, key);
}

void      gib_hash_set(gib_hash *hash, char *key, void *data)
{
	gib_list *l;
	gib_hash_node *n;

	n = GIB_HASH_NODE(gib_list_find(GIB_LIST(hash->base),
	                                gib_hash_find_callback,
	                                key));
	if (n)	{
		GIB_LIST(n)->data = data;
	} else {
		/* not using gib_list_add_end here as that would nest the
			 data one level, doing manual insert instead */
		n = gib_hash_node_new(key,data);
		l = gib_list_last(GIB_LIST(hash->base));

		GIB_LIST(n)->next = NULL;
		GIB_LIST(n)->prev = l;

		if (l)
			l->next = GIB_LIST(n);
	}
}

void     *gib_hash_get(gib_hash *hash, char *key)
{
	gib_list *n = gib_list_find(GIB_LIST(hash->base),gib_hash_find_callback,key);
	return n?n->data:NULL;
}

/* unused

void      gib_hash_remove(gib_hash *hash, char *key)
{
	gib_list *n = gib_list_find(GIB_LIST(hash->base), gib_hash_find_callback, key);
	if (n) {
		gib_list_unlink(GIB_LIST(hash->base), n);
    gib_hash_node_free(GIB_HASH_NODE(n->data));
  }

	return;
}

void      gib_hash_foreach(gib_hash *hash, void (*foreach_cb)(gib_hash_node *node, void *data), void *data)
{
	gib_hash_node *i, *next;
	for (i=hash->base; i; i=next) {
		next = GIB_HASH_NODE(GIB_LIST(i)->next);
		foreach_cb(i,data);
	}
	return;
}

*/
