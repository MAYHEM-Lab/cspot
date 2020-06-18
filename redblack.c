/*
# Software License Agreement (BSD License)
#
# Copyright (c) 2009-2012, Eucalyptus Systems, Inc.
# All rights reserved.
#
# Redistribution and use of this software in source and binary forms, with or
# without modification, are permitted provided that the following conditions
# are met:
#
#   Redistributions of source code must retain the above
#   copyright notice, this list of conditions and the
#   following disclaimer.
#
#   Redistributions in binary form must reproduce the above
#   copyright notice, this list of conditions and the
#   following disclaimer in the documentation and/or other
#   materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
*/
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef DEBUG_RB
#include "dlist.h" /* for printing */
#endif

#include "redblack.h"

int CompareKeyType(KEY_t op1, KEY_t op2)
{
	int i1, i2;
	int64_t i64_1, i64_2;
	double d1, d2;
	char *s1, *s2;
	/*
	 * return < 0 => op1 < op2
	 * return > 0 => op1 > op2
         * return == 0 => op1 == op2
         */
	switch (op1.type)
	{
	case K_INT:
		i1 = op1.key.i;
		i2 = op2.key.i;
		if (i1 < i2)
		{
			return (-1);
		}
		else if (i1 > i2)
		{
			return (1);
		}
		else
			return (0);
		break;
	case K_INT64:
		i64_1 = op1.key.i64;
		i64_2 = op2.key.i64;
		if (i64_1 < i64_2)
		{
			return (-1);
		}
		else if (i64_1 > i64_2)
		{
			return (1);
		}
		else
			return (0);
		break;
	case K_DOUBLE:
		d1 = op1.key.d;
		d2 = op2.key.d;
		if (d1 < d2)
		{
			return (-1);
		}
		else if (d1 > d2)
		{
			return (1);
		}
		else
			return (0);
		break;
	case K_STRING:
		/*
		 	 * need to do it this way in case the strings
			 * are the same since strcmp doesn't handle overlap
			 * if they overlap but are not the same the
			 * results will depend on strcmp
			 */
		s1 = op1.key.s;
		s2 = op2.key.s;
		if (s1 == s2)
		{
			return (0);
		}
		return (strcmp(s1, s2));
		break;
	default:
		fprintf(stderr,
				"CompareKeyType: bad type %d\n", op1.type);
		fflush(stderr);
		exit(1);
	}

	return (0);
}

RB *RBInit()
{
	RB *node;

	node = (RB *)malloc(sizeof(RB));
	if (node == NULL)
	{
		return (NULL);
	}
	memset(node, 0, sizeof(RB));

	return (node);
}

void RBFree(RB *node)
{
	free(node);
	return;
}

RB *RBTreeInit(unsigned char type)
{
	RB *node;

	node = RBInit();
	node->color = RB_GREEN; /* need this if the root rotates */
	node->key.type = type;

	return (node);
}

RB *RBGrandP(RB *node)
{
	if ((node == NULL) || (node->parent == NULL) ||
		(node->parent->parent == NULL) ||
		(node->parent->parent->color == RB_GREEN))
	{
		return (NULL);
	}

	return (node->parent->parent);
}

RB *RBUncle(RB *node)
{
	RB *gp;

	if (node == NULL)
	{
		return (NULL);
	}

	if (node->parent == NULL)
	{
		return (NULL);
	}

	gp = RBGrandP(node);
	if (gp == NULL)
	{
		return (NULL);
	}

	if (node->parent == gp->left)
	{
		return (gp->right);
	}
	else
	{
		return (gp->left);
	}
}

RB *RBInitTree()
{
	RB *tree;

	tree = RBInit();
	if (tree == NULL)
	{
		return (NULL);
	}
	tree->color = RB_GREEN; /* uninitialized */

	return (tree);
}

RB *RBAddTree(RB *parent, KEY_t key, Hval value, RB *top)
{
	RB *node;

	/*
	 * if this is the very root, use the left child
	 */
	if (parent->color == RB_GREEN)
	{
		/*
		 * if there isn't a node at the top, make one
		 */
		if (parent->left == NULL)
		{
			node = RBInit();
			if (node == NULL)
			{
				return (NULL);
			}
			node->key = key;
			node->value = value;
			node->parent = parent;
			parent->left = node;
			/*
			 * use prev and next of green node at top to
			 * be first and last
			 */
			parent->prev = node;
			parent->next = node;
			return (node);
		}
		else /* start down the garden path */
		{
			return (RBAddTree(parent->left, key, value, top));
		}
	}
	else if (CompareKeyType(key, parent->key) < 0)
	{
		if (parent->left == NULL)
		{
			node = RBInit();
			if (node == NULL)
			{
				return (NULL);
			}
			node->value = value;
			node->key = key;
			node->parent = parent;
			parent->left = node;

			/*
			 * insert in linked list
			 */
			if (parent->prev)
			{
				parent->prev->next = node;
			}
			node->prev = parent->prev;

			node->next = parent;
			parent->prev = node;

			/*
			 * new head of the list?
			 */
			if (top->prev == parent)
			{
				top->prev = node;
			}
			return (node);
		}
		else
		{
			return (RBAddTree(parent->left, key, value, top));
		}
	}
	else if (parent->right == NULL)
	{
		node = RBInit();
		if (node == NULL)
		{
			return (NULL);
		}
		node->value = value;
		node->key = key;
		parent->right = node;
		node->parent = parent;

		/*
		 * insert in linked list
		 */
		if (parent->next != NULL)
		{
			parent->next->prev = node;
		}
		node->next = parent->next;

		node->prev = parent;
		parent->next = node;

		if (top->next == parent)
		{
			top->next = node;
		}
		return (node);
	}
	else
	{
		return (RBAddTree(parent->right, key, value, top));
	}
}

void RBRotateLeft(RB *root)
{
	RB *root_parent = root->parent;
	RB *top;
	RB *root_left = root->left;
	RB *root_right = root->right;
	RB *new_root = root->right;
	RB *new_root_left = new_root->left;
	RB *new_root_right = new_root->right;

	if ((root_parent != NULL) && (root_parent->color == RB_GREEN))
	{
		top = root_parent;
		root_parent = NULL;
	}
	else
	{
		top = NULL;
	}

	root->right = new_root_left;
	if (new_root_left)
	{
		new_root_left->parent = root;
	}
	new_root->left = root;
	root->parent = new_root;

	if (root_parent != NULL)
	{
		if (root_parent->left == root)
		{
			root_parent->left = new_root;
		}
		else
		{
			root_parent->right = new_root;
		}
	}
	new_root->parent = root_parent;

	/*
	 * reset if the top of the tree rotates
	 */
	if (top != NULL)
	{
		top->left = new_root;
		new_root->parent = top;
	}

	return;
}

void RBRotateRight(RB *root)
{
	RB *root_parent = root->parent;
	RB *top;
	RB *root_left = root->left;
	RB *root_right = root->right;
	RB *new_root = root->left;
	RB *new_root_left = new_root->left;
	RB *new_root_right = new_root->right;

	if ((root_parent != NULL) && (root_parent->color == RB_GREEN))
	{
		top = root_parent;
		root_parent = NULL;
	}
	else
	{
		top = NULL;
	}

	root->left = new_root_right;
	if (new_root_right)
	{
		new_root_right->parent = root;
	}
	new_root->right = root;
	root->parent = new_root;

	if (root_parent != NULL)
	{
		if (root_parent->left == root)
		{
			root_parent->left = new_root;
		}
		else
		{
			root_parent->right = new_root;
		}
	}
	new_root->parent = root_parent;

	if (top != NULL)
	{
		top->left = new_root;
		new_root->parent = top;
	}

	return;
}

void RBInsert5(RB *node)
{
	RB *gp;

	gp = RBGrandP(node);

	if (node->parent->color != RB_GREEN)
	{
		node->parent->color = RB_BLACK;
	}

	if (gp && (node == node->parent->left) && (node->parent == gp->left))
	{
		gp->color = RB_RED;
		RBRotateRight(gp);
		return;
	}

	if (gp && (node == node->parent->right) && (node->parent == gp->right))
	{
		gp->color = RB_RED;
		RBRotateLeft(gp);
		return;
	}

	return;
}

void RBInsert4(RB *node)
{
	RB *gp;
	RB *new_node;
	int is_top;

	if (node->parent->color == RB_GREEN)
	{
		is_top = 1;
	}
	else
	{
		is_top = 0;
	}

	gp = RBGrandP(node);
	if (!is_top && (node == node->parent->right) &&
		(node->parent == gp->left))
	{
		RBRotateLeft(node->parent);
		new_node = node->left;
	}
	else if (!is_top && (node == node->parent->left) &&
			 (node->parent == gp->right))
	{
		RBRotateRight(node->parent);
		new_node = node->right;
	}
	else
	{
		new_node = node; /* skip to step 5 with "node" */
	}
	RBInsert5(new_node);

	return;
}

void RBInsert1(RB *node); /* forward def */

void RBInsert3(RB *node)
{
	RB *uncle;
	RB *gp;

	uncle = RBUncle(node);
	if ((uncle != NULL) && (uncle->color == RB_RED))
	{
		node->parent->color = RB_BLACK;
		uncle->color = RB_BLACK;
		gp = RBGrandP(node);
		gp->color = RB_RED;
		RBInsert1(gp);
		return;
	}

	RBInsert4(node);

	return;
}

void RBInsert2(RB *node)
{
	if (node->parent->color == RB_BLACK)
	{
		return;
	}

	RBInsert3(node);

	return;
}

void RBInsert1(RB *node)
{
	if (node->parent->color == RB_GREEN)
	{
		node->color = RB_BLACK;
		return;
	}

	RBInsert2(node);

	return;
}

void RBInsert(RB *tree, KEY_t key, Hval value)
{
	RB *node;

	node = RBAddTree(tree, key, value, tree);
	if (node == NULL)
	{
		fprintf(stderr, "RBInsert failed\n");
		fflush(stderr);
		exit(1);
	}

	node->color = RB_RED;

	RBInsert1(node);

	return;
}

RB *RBFindNode(RB *node, KEY_t key)
{
	if (node == NULL)
	{
		return (NULL);
	}
	if (CompareKeyType(node->key, key) == 0)
	{
		return (node);
	}

	if (CompareKeyType(key, node->key) < 0)
	{
		return (RBFindNode(node->left, key));
	}
	else
	{
		return (RBFindNode(node->right, key));
	}
}

RB *RBFind(RB *tree, KEY_t key)
{
	RB *node;

	if (tree->left == NULL)
	{
		return (NULL);
	}

	node = RBFindNode(tree->left, key);

	return (node);
}

RB *RBSibling(RB *node)
{
	if (node->parent->color == RB_GREEN)
	{
		return (NULL);
	}

	if (node == node->parent->left)
	{
		return (node->parent->right);
	}
	else
	{
		return (node->parent->left);
	}
}

void RBDelete1(RB *node);

void RBDelete6(RB *node)
{
	RB *sib;

	sib = RBSibling(node);
	sib->color = node->parent->color;
	node->parent->color = RB_BLACK;
	if (node == node->parent->left)
	{
		if (sib->right)
		{
			sib->right->color = RB_BLACK;
		}
		RBRotateLeft(node->parent);
	}
	else
	{
		if (sib->left)
		{
			sib->left->color = RB_BLACK;
		}
		RBRotateRight(node->parent);
	}

	return;
}

void RBDelete5(RB *node)
{
	RB *sib;

	sib = RBSibling(node);

	if (sib->color == RB_BLACK)
	{
		if ((node == node->parent->left) &&
			(!sib->right || (sib->right->color == RB_BLACK)) &&
			(sib->left && (sib->left->color == RB_RED)))
		{
			sib->color = RB_RED;
			if (sib->left)
			{
				sib->left->color = RB_BLACK;
			}
			RBRotateRight(sib);
		}
		else if ((node == node->parent->right) &&
				 (!sib->left || (sib->left->color == RB_BLACK)) &&
				 (sib->right && (sib->right->color == RB_RED)))
		{
			sib->color = RB_RED;
			if (sib->right)
			{
				sib->right->color = RB_BLACK;
			}
			RBRotateLeft(sib);
		}
	}
	RBDelete6(node);
	return;
}

void RBDelete4(RB *node)
{
	RB *sib;

	sib = RBSibling(node);

	if ((node->parent->color == RB_RED) &&
		(sib->color == RB_BLACK) &&
		(!sib->left || (sib->left->color == RB_BLACK)) &&
		(!sib->right || (sib->right->color == RB_BLACK)))
	{
		sib->color = RB_RED;
		node->parent->color = RB_BLACK;
	}
	else
	{
		RBDelete5(node);
	}
	return;
}

void RBDelete3(RB *node)
{
	RB *sib;

	sib = RBSibling(node);
	if ((node->parent->color == RB_BLACK) &&
		(sib->color == RB_BLACK) &&
		(!sib->left || (sib->left->color == RB_BLACK)) &&
		(!sib->right || (sib->right->color == RB_BLACK)))
	{
		sib->color = RB_RED;
		RBDelete1(node->parent);
	}
	else
	{
		RBDelete4(node);
	}
	return;
}

void RBDelete2(RB *node)
{
	RB *sib;

	sib = RBSibling(node);
	if (sib->color == RB_RED)
	{
		node->parent->color = RB_RED;
		sib->color = RB_BLACK;
		if (node == node->parent->left)
		{
			RBRotateLeft(node->parent);
		}
		else
		{
			RBRotateRight(node->parent);
		}
	}
	RBDelete3(node);
	return;
}

void RBDelete1(RB *node)
{
	if (node->parent->color != RB_GREEN)
	{
		RBDelete2(node);
	}

	return;
}

void RBDeleteOneChild(RB *node)
{
	/*
	 * assumes that node has only one non-null child
	 */
	RB *child;

	if (node->left)
	{
		child = node->left;
	}
	else
	{
		child = node->right;
	}

	if (child == NULL)
	{
		/*
		 * child == NULL means child is black
		 *
		 * parent black needs help, but parent red is
		 * just a delete
		 */
		if (node->color == RB_BLACK)
		{
			RBDelete1(node);
		}
		if (node == node->parent->left)
		{
			node->parent->left = NULL;
		}
		else
		{
			node->parent->right = NULL;
		}
		if (node->next)
		{
			node->next->prev = node->prev;
		}
		if (node->prev)
		{
			node->prev->next = node->next;
		}
		RBFree(node);
		return;
	}

	/*
	 * splice out the node for the child
	 */

	child->parent = node->parent;
	if (node->parent->left == node)
	{
		node->parent->left = child;
	}
	else
	{
		node->parent->right = child;
	}

	if (node->next && (node->next != child))
	{
		node->next->prev = child;
	}
	if (node->next != child)
	{
		child->next = node->next;
	}
	if (node->prev && (node->prev != child))
	{
		node->prev->next = child;
	}
	if (node->prev != child)
	{
		child->prev = node->prev;
	}

	if (node->parent->color == RB_GREEN)
	{
		if (node->parent->next == node)
		{
			node->parent->next = child;
		}
		if (node->parent->prev == node)
		{
			node->parent->prev = child;
		}
	}

	/*
	 * node should be configured out of tree at this point and
	 * child substituted in
	 */

	if (node->color == RB_BLACK)
	{
		if (child->color == RB_RED)
		{
			child->color = RB_BLACK;
		}
		else
		{
			RBDelete1(child);
		}
	}
	RBFree(node);
	return;
}

RB *RBSwapMaxLeft(RB *node)
{
	RB *curr;
	KEY_t tempk;
	Hval tempv;

	if (node->left == NULL)
	{
		return (NULL);
	}

	curr = node->left;

	while (curr->right != NULL)
	{
		curr = curr->right;
	}

	tempk = curr->key;
	curr->key = node->key;
	node->key = tempk;

	tempv = curr->value;
	curr->value = node->value;
	node->value = tempv;

	return (curr);
}

void RBDeleteNode(RB *tree, RB *node)
{
	RB *target;

	if (node->left && node->right)
	{
		target = RBSwapMaxLeft(node);
		/*
		 * may need to swap the head and tail
		 */
		if (tree->prev == target)
		{
			tree->prev = node;
		}
		if (tree->next == target)
		{
			tree->next = node;
		}
	}
	else
	{
		target = node;
	}

	RBDeleteOneChild(target);

	return;
}

void RBDelete(RB *tree, RB *node)
{
	if (tree->prev && (tree->prev == node))
	{
		tree->prev = node->next;
	}
	if (tree->next && (tree->next == node))
	{
		tree->next = node->prev;
	}
	RBDeleteNode(tree, node);

	return;
}

void RBDeleteTree(RB *tree)
{
	RB *node;
	RB *next;

	node = tree->prev;
	if (node != NULL)
	{
		next = node->next;
	}

	while (node)
	{
		RBFree(node);
		node = next;
		if (node == NULL)
		{
			break;
		}
		next = node->next;
	}

	RBFree(tree);
}

/*
 * type specificity
 */
RB *RBInitD()
{
	RB *rb = RBTreeInit(K_DOUBLE);

	return (rb);
}

void RBDestroyD(RB *rb)
{
	return (RBDeleteTree(rb));
}

void RBDeleteD(RB *rb, RB *node)
{
	return (RBDelete(rb, node));
}

RB *RBFindD(RB *rb, double dkey)
{
	KEY_t key;

	key.type = K_DOUBLE;
	key.key.d = dkey;
	return (RBFind(rb, key));
}

void RBInsertD(RB *rb, double dkey, Hval value)
{
	KEY_t key;

	key.type = K_DOUBLE;
	key.key.d = dkey;
	return (RBInsert(rb, key, value));
}

RB *RBInitI()
{
	RB *rb = RBTreeInit(K_INT);
	return (rb);
}

void RBDestroyI(RB *rb)
{
	return (RBDeleteTree(rb));
}

void RBDeleteI(RB *rb, RB *node)
{
	return (RBDelete(rb, node));
}

RB *RBFindI(RB *rb, int ikey)
{
	KEY_t key;

	key.type = K_INT;
	key.key.i = ikey;
	return (RBFind(rb, key));
}

void RBInsertI(RB *rb, int ikey, Hval value)
{
	KEY_t key;

	key.type = K_INT;
	key.key.i = ikey;
	return (RBInsert(rb, key, value));
}

RB *RBInitI64()
{
	RB *rb = RBTreeInit(K_INT64);
	return (rb);
}

void RBDestroyI64(RB *rb)
{
	return (RBDeleteTree(rb));
}

void RBDeleteI64(RB *rb, RB *node)
{
	return (RBDelete(rb, node));
}

RB *RBFindI64(RB *rb, int64_t ikey)
{
	KEY_t key;

	key.type = K_INT64;
	key.key.i64 = ikey;
	return (RBFind(rb, key));
}

void RBInsertI64(RB *rb, int64_t ikey, Hval value)
{
	KEY_t key;

	key.type = K_INT64;
	key.key.i64 = ikey;
	return (RBInsert(rb, key, value));
}

RB *RBInitS()
{
	RB *rb = RBTreeInit(K_STRING);
	return (rb);
}

void RBDestroyS(RB *rb)
{
	return (RBDeleteTree(rb));
}

void RBDeleteS(RB *rb, RB *node)
{
	return (RBDelete(rb, node));
}

RB *RBFindS(RB *rb, char *skey)
{
	KEY_t key;

	key.type = K_STRING;
	key.key.s = skey;
	return (RBFind(rb, key));
}

void RBInsertS(RB *rb, char *skey, Hval value)
{
	KEY_t key;

	key.type = K_STRING;
	key.key.s = skey;
	return (RBInsert(rb, key, value));
}

#ifdef DEBUG_RB

int RBTestGP(RB *node)
{
	RB *parent = node->parent;
	RB *gp = RBGrandP(node);

	if (gp == NULL)
	{
		return (1);
	}

	if (parent == gp->left)
	{
		return (1);
	}

	if (parent == gp->right)
	{
		return (1);
	}

	return (0);
}

int RBTestGPs(RB *tree)
{
	int status;
	RB *top;

	top = tree->left;

	if (top->left)
	{
		status = RBTestGP(top->left);
		if (status == 0)
		{
			return (0);
		}
	}
	if (top->right)
	{
		status = RBTestGP(top->right);
		if (status == 0)
		{
			return (0);
		}
	}

	return (1);
}

void RBTestChildColor(RB *tree, int *both_black)
{
	if (tree == NULL)
	{
		return;
	}
	/*
	 * both children of red much be black
	 *
	 * both_black initialized to TRUE before call at top
	 *
	 * null child is black
	 */
	if (tree->color == RB_RED)
	{
		if (tree->left)
		{
			if (tree->left->color != RB_BLACK)
			{
				*both_black = 0;
				return;
			}
		}
		if (tree->right)
		{
			if (tree->right->color != RB_BLACK)
			{
				*both_black = 0;
				return;
			}
		}
	}
	else
	{
		RBTestChildColor(tree->left, both_black);
		if (*both_black == 0)
		{
			return;
		}
		RBTestChildColor(tree->right, both_black);
		if (*both_black == 0)
		{
			return;
		}
	}

	return;
}

int RBTestPaths(RB *tree, int *black_count, int *status)
{
	/*
	 * all simple paths from a node to its descendant leaves
	 * must contain the same number of black nodes
	 *
	 * assume *status == 1 to start
	 */
	int left_black;
	int right_black;

	if (tree->left == NULL)
	{
		if (tree->color == RB_BLACK)
		{
			left_black = 2;
		}
		else
		{
			left_black = 1;
		}
	}
	else
	{
		RBTestPaths(tree->left, &left_black, status);
		if (*status == 0)
		{
			return (0);
		}
		if (tree->color == RB_BLACK)
		{
			left_black++;
		}
	}

	if (tree->right == NULL)
	{
		if (tree->color == RB_BLACK)
		{
			right_black = 2;
		}
		else
		{
			right_black = 1;
		}
	}
	else
	{
		RBTestPaths(tree->right, &right_black, status);
		if (*status == 0)
		{
			return (0);
		}
		if (tree->color == RB_BLACK)
		{
			right_black++;
		}
	}

	if (left_black != right_black)
	{
		*status = 0;
	}
	else
	{
		*black_count = left_black; /* they are the same */
	}

	return (0);
}

int RBTestInvariants(RB *tree)
{
	int both_black;
	int all_black_same;
	int black_count;

	if ((tree->prev == NULL) && (tree->next == NULL) && (tree->left == NULL))
	{
		return (1);
	}

	/*
	 * tree left is the top of the tree
	 */
	if (tree->left->color != RB_BLACK)
	{
		fprintf(stderr, "root isn't black\n");
		return (0);
	}

	both_black = 1;
	RBTestChildColor(tree->left, &both_black);

	if (both_black == 0)
	{
		fprintf(stderr, "both black children of red fails\n");
		return (0);
	}

	all_black_same = 1;
	RBTestPaths(tree->left, &black_count, &all_black_same);

	if (all_black_same == 0)
	{
		fprintf(stderr, "black path invariant fails\n");
		return (0);
	}

	return (1);
}

void RBPrintNode(RB *node)
{
	switch (node->key.type)
	{
	case K_INT:
		printf("n: 0x%lx, key: %d color: %d, l: 0x%lx, r: 0x%lx p: 0x%lx\n",
			   (unsigned long)node,
			   node->key.key.i,
			   node->color,
			   (unsigned long)node->left,
			   (unsigned long)node->right,
			   (unsigned long)node->parent);
		break;
	case K_DOUBLE:
		printf("n: 0x%lx, key: %f color: %d, l: 0x%lx, r: 0x%lx p: 0x%lx\n",
			   (unsigned long)node,
			   node->key.key.d,
			   node->color,
			   (unsigned long)node->left,
			   (unsigned long)node->right,
			   (unsigned long)node->parent);
		break;
	case K_STRING:
		printf("n: 0x%lx, key: %s color: %d, l: 0x%lx, r: 0x%lx p: 0x%lx\n",
			   (unsigned long)node,
			   node->key.key.s,
			   node->color,
			   (unsigned long)node->left,
			   (unsigned long)node->right,
			   (unsigned long)node->parent);
		break;
	}

	return;
}

void RBPrintTree(RB *tree)
{
	Dlist *que;
	Hval value;
	RB *node;

	que = DlistInit();
	if (que == NULL)
	{
		return;
	}

	value.v = (void *)tree->left;
	DlistAppend(que, value);

	while (que->count != 0)
	{
		node = (RB *)(que->first->value.v);
		RBPrintNode(node);
		DlistDelete(que, que->first);
		if (node->left)
		{
			value.v = (void *)node->left;
			DlistAppend(que, value);
		}
		if (node->right)
		{
			value.v = (void *)node->right;
			DlistAppend(que, value);
		}
	}

	DlistRemove(que);

	return;
}
#endif

#ifdef TEST

int rbyan()
{
	return (0);
}

#define MAX_DEL_COUNT (40)
#define BIG_COUNT (10000)

int main(int argc, char *argv[])
{

	RB *tree;
	int i;
	double r;
	int status;
	RB *curr;
	RB *next;
	RB *node;
	double *del_key;
	int del_count;
	int count;

	tree = RBInitD();
	if (tree == NULL)
	{
		fprintf(stderr, "failed to make tree\n");
		exit(1);
	}

	del_count = 0;
	del_key = (double *)malloc(MAX_DEL_COUNT * sizeof(double));

	for (i = 0; i < 200; i++)
	{
		r = drand48();
		printf("inserting: %f\n", r);
		RBInsertD(tree, r, (Hval)0);
		//		RBPrintTree(tree);
		status = RBTestGPs(tree);
		if (status == 0)
		{
			printf("RB tree fails GP test\n");
			break;
		}
		status = RBTestInvariants(tree);
		if (status == 0)
		{
			printf("RB tree fails invarant test\n");
			break;
		}

		if ((drand48() < 0.1) && (del_count < MAX_DEL_COUNT))
		{
			del_key[del_count] = r;
			del_count++;
		}
	}

	curr = tree->prev;
	while (curr != NULL)
	{
		printf("sorted: %f\n", K_D(curr->key));
		curr = curr->next;
	}

	for (i = 0; i < del_count; i++)
	{
		printf("searching for %f\n", del_key[i]);
		node = RBFindD(tree, del_key[i]);
		if (node != NULL)
		{
			printf("\tfound\n");
		}
		else
		{
			printf("\tnot found\n");
			exit(1);
		}

		printf("deleteing %f\n", del_key[i]);
		RBDeleteD(tree, node);
		//		RBPrintTree(tree);
		status = RBTestInvariants(tree);
		if (status == 0)
		{
			printf("RB tree fails invarant test\n");
			exit(1);
		}
	}

	/*
	 * let's try deleting the root
	 */
	printf("deleting root: %f\n", K_D(tree->left->key));
	RBDeleteD(tree, tree->left);
	status = RBTestInvariants(tree);
	if (status == 0)
	{
		printf("RB tree fails invarant test after root\n");
		exit(1);
	}
	printf("new root: %f\n", K_D(tree->left->key));
	curr = tree->prev;
	while (curr != NULL)
	{
		printf("sorted: %f\n", K_D(curr->key));
		curr = curr->next;
	}

	printf("deleteing tree\n");
	RBDestroyD(tree);

	tree = RBInitD();
	if (tree == NULL)
	{
		fprintf(stderr, "couldn't get new tree\n");
		exit(1);
	}

	printf("doing big insert\n");
	for (i = 0; i < BIG_COUNT; i++)
	{
		r = drand48();
		RBInsertD(tree, r, (Hval)0);
		status = RBTestInvariants(tree);
		if (status == 0)
		{
			printf("RB tree fails big invarant test\n");
			break;
		}
	}

	count = BIG_COUNT;
	printf("doing big delete test\n");

	while (count > 10)
	{
		r = drand48();
		curr = tree->prev;
		for (i = 0; i < (int)(r * count); i++)
		{
			if (CompareKeyType(tree->prev->key, tree->prev->next->key) > 0)
			{
				printf("bad sequence\n");
				exit(1);
			}
			curr = curr->next;
		}
		printf("deleting: %20.30e\n", K_D(curr->key));
		RBDeleteD(tree, curr);
		status = RBTestInvariants(tree);
		if (status == 0)
		{
			printf("RB tree fails big delete %d\n", count);
			exit(1);
		}
		count--;
	}

	curr = tree->prev;
	while (curr != NULL)
	{
		printf("sorted: %f\n", K_D(curr->key));
		curr = curr->next;
	}

	printf("deleteing first\n");
	RBDeleteD(tree, RB_FIRST(tree));
	status = RBTestInvariants(tree);
	if (status == 0)
	{
		fprintf(stderr, "first delete fails\n");
		exit(1);
	}
	curr = RB_FIRST(tree);
	while (curr != NULL)
	{
		printf("sorted: %f\n", K_D(curr->key));
		curr = curr->next;
	}

	printf("deleteing last\n");
	RBDeleteD(tree, RB_LAST(tree));
	status = RBTestInvariants(tree);
	if (status == 0)
	{
		fprintf(stderr, "last delete fails\n");
		exit(1);
	}
	curr = RB_LAST(tree);
	while (curr != NULL)
	{
		printf("sorted: %f\n", K_D(curr->key));
		curr = curr->prev;
	}

	curr = RB_FIRST(tree);
	while (curr != NULL)
	{
		next = curr->next;
		printf("deleteing %f\n", K_D(curr->key));
		RBDeleteD(tree, curr);
		status = RBTestInvariants(tree);
		if (status == 0)
		{
			fprintf(stderr, "intermediate delete failed\n");
			exit(1);
		}
		curr = next;
	}

	return (0);
}

#endif
