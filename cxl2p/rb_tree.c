#include "rb_tree.h"

// left rotation
static void rbtree_left_rotate(RedBlackTree *T, rbtree_node *x)
{
	rbtree_node *y = x->right;
	// 1
	x->right = y->left;
	if (y->left != T->nil)
		y->left->parent = x;

	// 2
	y->parent = x->parent;
	if (x->parent == T->nil)
		T->root = y;
	else if (x == x->parent->left)
		x->parent->left = y;
	else
		x->parent->right = y;

	// 3
	y->left = x;
	x->parent = y;
}
// right rotation
static void rbtree_right_rotate(RedBlackTree *T, rbtree_node *y)
{
	rbtree_node *x = y->left;
	y->left = x->right;
	if (x->right != T->nil)
		x->right->parent = y;
	x->parent = y->parent;
	if (y->parent == T->nil)
		T->root = x;
	else if (y == y->parent->right)
		y->parent->right = x;
	else
		y->parent->left = x;
	x->right = y;
	y->parent = x;
}

static void rbtree_insert_fixup(RedBlackTree *T, rbtree_node *z)
{
	while (z->parent->color == RED)
	{
		if (z->parent == z->parent->parent->left)
		{
			// The parent of z is the left subtree of the grandfather
			rbtree_node *y = z->parent->parent->right;
			if (y->color == RED)
			{
				y->color = BLACK;
				z->parent->color = BLACK;
				z->parent->parent->color = RED;

				// You have to make sure Z is red every time you go through it.
				z = z->parent->parent; // z->color == RED
			}
			else
			{
				// This cannot be done in one step, it must be done in the middle:
				//  The state with a large number of nodes on the left is easy to rotate.

				if (z->parent->right == z)
				{
					z = z->parent;
					rbtree_left_rotate(T, z);
				}

				// The number of nodes to the left of the root node is large
				// Especially if you have two red nodes bordering each other.
				// Need to rotate.

				// Change the color, and then rotate
				z->parent->color = BLACK;
				z->parent->parent->color = RED;
				rbtree_right_rotate(T, z->parent->parent);
			}
		}
		else
		{ // The parent of z is the right subtree of the grandfather
			rbtree_node *y = z->parent->parent->left;
			if (y->color == RED) // Uncle node is red
			{
				z->parent->parent->color = RED;
				z->parent->color = BLACK;
				y->color = BLACK;

				z = z->parent->parent; // z->color == RED
			}
			else
			{
				if (z == z->parent->left)
				{
					z = z->parent;
					rbtree_right_rotate(T, z);
				}
				z->parent->color = BLACK;
				z->parent->parent->color = RED;
				rbtree_left_rotate(T, z->parent->parent);
			}
		}
	}
	T->root->color = BLACK;
}

void rbtree_insert(RedBlackTree *T, rbtree_node *z)
{
	rbtree_node *pre = T->nil;
	rbtree_node *cur = T->root;
	while (cur != T->nil)
	{
		pre = cur;
		if (z->key > cur->key)
			cur = cur->right;
		else if (z->key < cur->key)
			cur = cur->left;
		else
			return;
	}

	z->parent = pre;
	if (pre == T->nil)
	{
		T->root = z;
	}
	else
	{
		if (pre->key > z->key)
			pre->left = z;
		else
			pre->right = z;
	}
	z->left = T->nil;
	z->right = T->nil;

	z->color = RED;
	rbtree_insert_fixup(T, z);
}

rbtree_node *rbtree_mini(RedBlackTree *T, rbtree_node *x)
{
	while (x->left != T->nil)
	{
		x = x->left;
	}
	return x;
}

rbtree_node *rbtree_maxi(RedBlackTree *T, rbtree_node *x)
{
	while (x->right != T->nil)
	{
		x = x->right;
	}
	return x;
}

rbtree_node *rbtree_successor(RedBlackTree *T, rbtree_node *x)
{
	rbtree_node *y = x->parent;
	if (x->right != T->nil)
	{
		return rbtree_mini(T, x->right);
	}
	while ((y != T->nil) && (x == y->right))
	{
		x = y;
		y = y->parent;
	}
	return y;
}

// fixup function
void rbtree_delete_fixup(RedBlackTree *T, rbtree_node *x)
{

	while ((x != T->root) && (x->color == BLACK))
	{
		if (x == x->parent->left)
		{

			rbtree_node *w = x->parent->right;
			if (w->color == RED)
			{
				w->color = BLACK;
				x->parent->color = RED;

				rbtree_left_rotate(T, x->parent);
				w = x->parent->right;
			}

			if ((w->left->color == BLACK) && (w->right->color == BLACK))
			{
				w->color = RED;
				x = x->parent;
			}
			else
			{

				if (w->right->color == BLACK)
				{
					w->left->color = BLACK;
					w->color = RED;
					rbtree_right_rotate(T, w);
					w = x->parent->right;
				}

				w->color = x->parent->color;
				x->parent->color = BLACK;
				w->right->color = BLACK;
				rbtree_left_rotate(T, x->parent);

				x = T->root;
			}
		}
		else
		{

			rbtree_node *w = x->parent->left;
			if (w->color == RED)
			{
				w->color = BLACK;
				x->parent->color = RED;
				rbtree_right_rotate(T, x->parent);
				w = x->parent->left;
			}

			if ((w->left->color == BLACK) && (w->right->color == BLACK))
			{
				w->color = RED;
				x = x->parent;
			}
			else
			{

				if (w->left->color == BLACK)
				{
					w->right->color = BLACK;
					w->color = RED;
					rbtree_left_rotate(T, w);
					w = x->parent->left;
				}

				w->color = x->parent->color;
				x->parent->color = BLACK;
				w->left->color = BLACK;
				rbtree_right_rotate(T, x->parent);

				x = T->root;
			}
		}
	}

	x->color = BLACK;
}

rbtree_node *rbtree_delete(RedBlackTree *T, rbtree_node *z)
{
	rbtree_node *y = T->nil;
	rbtree_node *x = T->nil;

	if ((z->left == T->nil) || (z->right == T->nil))
	{
		y = z;
	}
	else
	{
		y = rbtree_successor(T, z);
	}

	if (y->left != T->nil)
		x = y->left;
	else if (y->right != T->nil)
		x = y->right;
	x->parent = y->parent;
	if (y->parent == T->nil)
		T->root = x;
	else if (y == y->parent->left)
		y->parent->left = x;
	else
		y->parent->right = x;
	if (y != z)
	{
		z->key = y->key;
		z->in_flash = y->in_flash;
		z->addr = y->addr;
	}

	if (y->color == BLACK)
	{
		rbtree_delete_fixup(T, x);
	}

	return y;
}

rbtree_node *rbtree_search(RedBlackTree *T, KEY_TYPE key)
{

	rbtree_node *node = T->root;
	rbtree_node *closest = T->nil;
	while (node != T->nil)
	{
		if (key == node->key)
		{
			return node; // Found an exact match
		}
		else if (key > node->key)
		{
			closest = node; // Update closest when moving left
			node = node->right;
		}
		else
		{
			node = node->left;
		}
	}
	// If we didn't find an exact match, return the closest node found
	return closest;
}

void rbtree_traversal(RedBlackTree *T, rbtree_node *node)
{
	if (node != T->nil)
	{
		rbtree_traversal(T, node->left);
		// printf("key:%d,in_flash:%d\n", node->key,node->in_flash);
		rbtree_traversal(T, node->right);
	}
}

// return all keys
void rbtree_get_keys(RedBlackTree *T, rbtree_node *node, KeyArray *result)
{
	if (node != T->nil)
	{
		rbtree_get_keys(T, node->left, result);
		result->len++;
		result->keys[result->len - 1] = node->key;
		result->in_flash[result->len - 1] = node->in_flash;
		rbtree_get_keys(T, node->right, result);
	}
	return;
}

void rbtree_deleteByKey(RedBlackTree *T, KEY_TYPE key)
{
	// printf("delete %lu in grt\n",key);
	rbtree_node *nodeToDelete = rbtree_search(T, key);
	if (nodeToDelete != T->nil)
	{
		rbtree_delete(T, nodeToDelete);
	}
}
