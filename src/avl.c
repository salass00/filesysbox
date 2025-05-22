/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2025 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#ifdef ENABLE_CHARSET_CONVERSION
#include "filesysbox_internal.h"

/* 
 * AVL implementation based on information from:
 * https://en.wikipedia.org/wiki/AVL_tree
 */

static void rotate_left(struct FbxAVL *x, struct FbxAVL *z) {
	struct FbxAVL *t = z->left;
	x->right = t;
	if (t != NULL) t->parent = x;
	z->left = x;
	x->parent = z;
}

static void rotate_right(struct FbxAVL *x, struct FbxAVL *z) {
	struct FbxAVL *t = z->right;
	x->left = t;
	if (t != NULL) t->parent = x;
	z->right = x;
	x->parent = z;
}

static void retrace(struct FbxAVL **root, struct FbxAVL *z) {
	struct FbxAVL *g, *n, *x, *y;

	for (x = z->parent; x != NULL; x = z->parent) {
		if (z == x->right) {
			if (x->balance > 0) {
				g = x->parent;

				if (z->balance < 0) {
					y = z->left;

					rotate_right(z, y);
					rotate_left(x, y);

					if (y->balance == 0) {
						x->balance = 0;
						z->balance = 0;
					} else {
						if (y->balance > 0) {
							x->balance = -1;
							z->balance = 0;
						} else {
							x->balance = 0;
							z->balance = 1;
						}
						y->balance = 0;
					}
					n = y;
				} else {
					rotate_left(x, z);

					//assert(z->balance != 0);
					x->balance = 0;
					z->balance = 0;
					n = z;
				}
			} else if (x->balance < 0) {
				x->balance = 0;
				break;
			} else {
				x->balance = 1;
				z = x;
				continue;
			}
		} else {
			if (x->balance < 0) {
				g = x->parent;

				if (z->balance > 0) {
					y = z->right;

					rotate_left(z, y);
					rotate_right(x, y);

					if (y->balance == 0) {
						x->balance = 0;
						z->balance = 0;
					} else {
						if (y->balance < 0) {
							x->balance = 1;
							z->balance = 0;
						} else {
							x->balance = 0;
							z->balance = -1;
						}
						y->balance = 0;
					}
					n = y;
				} else {
					rotate_right(x, z);

					//assert(n->balance != 0);
					x->balance = 0;
					z->balance = 0;
					n = z;
				}
			} else if (x->balance > 0) {
				x->balance = 0;
				break;
			} else {
				x->balance = -1;
				z = x;
				continue;
			}
		}

		n->parent = g;
		if (g != NULL) {
			if (x == g->left)
				g->left = n;
			else
				g->right = n;
		} else
			*root = n;

		break;
	}
}

static BOOL insert(struct FbxAVL **root, struct FbxAVL *n) {
	struct FbxAVL *p = *root;

	n->left    = NULL;
	n->right   = NULL;
	n->balance = 0;

	if (p == NULL) {
		*root = n;
		n->parent = p;
		return TRUE;
	}

	for (;;) {
		if (n->unicode == p->unicode)
			return FALSE;

		if (n->unicode < p->unicode) {
			if (p->left != NULL) {
				p = p->left;
				continue;
			}
			p->left = n;
		} else {
			if (p->right != NULL) {
				p = p->right;
				continue;
			}
			p->right = n;
		}

		n->parent = p;
		retrace(root, n);

		return TRUE;
	}
}

void FbxSetupAVL(struct FbxFS *fs) {
	struct FbxAVL *n;
	ULONG unicode;
	int i, local;

	for (i = 0; i < 0x80; i++) {
		local = 0x80 + i;
		unicode = fs->maptable[local];

		n = &fs->avlbuf[i];
		n->unicode = unicode;
		n->local = local;

		insert(&fs->maptree, n);
	}
}

#endif /* ENABLE_CHARSET_CONVERSION */

