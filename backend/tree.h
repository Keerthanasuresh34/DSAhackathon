#ifndef TREE_H
#define TREE_H

#include <stdio.h>

#define MAX_LABEL_LENGTH 32
#define MAX_NODES 2000

/* Error codes reported via the `error` out-parameter in buildTreeFromTokens */
#define TREE_OK                 0
#define TREE_ERR_INVALID_ARG    1
#define TREE_ERR_INVALID_LABEL  2
#define TREE_ERR_TRUNCATED      3
#define TREE_ERR_MAX_NODES      4
#define TREE_ERR_MEMORY         5

typedef struct TreeNode {
    char label[MAX_LABEL_LENGTH];
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

/*
 * Allocates and initializes a new TreeNode with the given label.
 * Labels must be non-NULL, non-empty, not equal to "#", and at most
 * MAX_LABEL_LENGTH - 1 (31) characters long.
 * Returns NULL on invalid input or memory allocation failure.
 */
TreeNode *createNode(const char *label);

/*
 * Recursively constructs a binary tree from an array of preorder tokens
 * where "#" denotes a NULL (missing) child.
 * Advances *position as tokens are consumed and writes TREE_OK (0) or a
 * non-zero error code to *error (if non-NULL).
 * Frees all partially allocated nodes and returns NULL if any error occurs
 * or if the tree exceeds MAX_NODES actual nodes.
 */
TreeNode *buildTreeFromTokens(
    char **tokens,
    int tokenCount,
    int *position,
    int *error
);

/*
 * Writes space-separated node labels to `output` using recursive traversals.
 * Handles NULL trees and NULL output streams safely.
 */
void preorder(TreeNode *root, FILE *output);
void inorder(TreeNode *root, FILE *output);
void postorder(TreeNode *root, FILE *output);

/*
 * Displays the tree in a readable indented branch format showing [Root],
 * [L], [R], missing (#) sibling markers where useful, and empty trees.
 */
void displayTree(TreeNode *root, FILE *output);

/*
 * Recursively frees every node in the tree using postorder traversal.
 * Safe to call with NULL.
 */
void freeTree(TreeNode *root);

/*
 * Compares two binary trees recursively for identical structure and labels
 * (same labels, same left/right child links, and same missing branches).
 * Returns 1 if identical, 0 otherwise.
 */
int compareTrees(TreeNode *tree1, TreeNode *tree2);

/*
 * Recursively counts the total number of non-NULL nodes in the tree.
 * Returns 0 for an empty (NULL) tree.
 */
int countNodes(TreeNode *root);

#endif
