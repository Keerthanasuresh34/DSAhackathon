#include "tree.h"

#include <stdlib.h>
#include <string.h>

/*
 * Validates that a node label is non-NULL, non-empty, not the "#" NULL marker,
 * and fits within MAX_LABEL_LENGTH (including the null terminator).
 */
static int isValidLabel(const char *label) {
    size_t len = 0;

    if (label == NULL || label[0] == '\0') {
        return 0;
    }

    if (strcmp(label, "#") == 0) {
        return 0;
    }

    while (len < MAX_LABEL_LENGTH && label[len] != '\0') {
        len++;
    }

    return len < MAX_LABEL_LENGTH;
}

TreeNode *createNode(const char *label) {
    TreeNode *node;
    size_t len;

    if (!isValidLabel(label)) {
        return NULL;
    }

    node = (TreeNode *)malloc(sizeof(TreeNode));
    if (node == NULL) {
        return NULL;
    }

    len = strlen(label);
    memcpy(node->label, label, len + 1);
    node->left = NULL;
    node->right = NULL;

    return node;
}

/*
 * Recursive helper for buildTreeFromTokens.
 * Tracks the running count of created nodes in *nodeCount to enforce
 * MAX_NODES and cleans up all partial allocations whenever a failure occurs.
 */
static TreeNode *buildTreeRecursive(
    char **tokens,
    int tokenCount,
    int *position,
    int *nodeCount,
    int *error
) {
    const char *token;
    TreeNode *node;

    if (*position >= tokenCount) {
        *error = TREE_ERR_TRUNCATED;
        return NULL;
    }

    token = tokens[*position];
    if (token == NULL) {
        *error = TREE_ERR_INVALID_LABEL;
        return NULL;
    }

    (*position)++;

    /* "#" marks an intentionally absent branch (NULL child pointer). */
    if (strcmp(token, "#") == 0) {
        return NULL;
    }

    if (*nodeCount >= MAX_NODES) {
        *error = TREE_ERR_MAX_NODES;
        return NULL;
    }

    if (!isValidLabel(token)) {
        *error = TREE_ERR_INVALID_LABEL;
        return NULL;
    }

    node = createNode(token);
    if (node == NULL) {
        *error = TREE_ERR_MEMORY;
        return NULL;
    }

    (*nodeCount)++;

    /* Recursively build left subtree; free partial tree if an error happens. */
    node->left = buildTreeRecursive(tokens, tokenCount, position, nodeCount, error);
    if (*error != TREE_OK) {
        freeTree(node);
        return NULL;
    }

    /* Recursively build right subtree; free partial tree if an error happens. */
    node->right = buildTreeRecursive(tokens, tokenCount, position, nodeCount, error);
    if (*error != TREE_OK) {
        freeTree(node);
        return NULL;
    }

    return node;
}

TreeNode *buildTreeFromTokens(
    char **tokens,
    int tokenCount,
    int *position,
    int *error
) {
    int localError = TREE_OK;
    int *errPtr = (error != NULL) ? error : &localError;
    int nodeCount = 0;

    *errPtr = TREE_OK;

    if (tokens == NULL || tokenCount <= 0 || position == NULL || *position < 0) {
        *errPtr = TREE_ERR_INVALID_ARG;
        return NULL;
    }

    if (*position >= tokenCount) {
        *errPtr = TREE_ERR_TRUNCATED;
        return NULL;
    }

    return buildTreeRecursive(tokens, tokenCount, position, &nodeCount, errPtr);
}

/*
 * Prints a single node label with single-space separation between tokens.
 */
static void writeTraversalLabel(const char *label, FILE *output, int *isFirst) {
    if (!(*isFirst)) {
        fputc(' ', output);
    }
    fputs(label, output);
    *isFirst = 0;
}

/* Recursive preorder traversal: Root -> Left -> Right. */
static void preorderRecursive(TreeNode *root, FILE *output, int *isFirst) {
    if (root == NULL) {
        return;
    }
    writeTraversalLabel(root->label, output, isFirst);
    preorderRecursive(root->left, output, isFirst);
    preorderRecursive(root->right, output, isFirst);
}

/* Recursive inorder traversal: Left -> Root -> Right. */
static void inorderRecursive(TreeNode *root, FILE *output, int *isFirst) {
    if (root == NULL) {
        return;
    }
    inorderRecursive(root->left, output, isFirst);
    writeTraversalLabel(root->label, output, isFirst);
    inorderRecursive(root->right, output, isFirst);
}

/* Recursive postorder traversal: Left -> Right -> Root. */
static void postorderRecursive(TreeNode *root, FILE *output, int *isFirst) {
    if (root == NULL) {
        return;
    }
    postorderRecursive(root->left, output, isFirst);
    postorderRecursive(root->right, output, isFirst);
    writeTraversalLabel(root->label, output, isFirst);
}

void preorder(TreeNode *root, FILE *output) {
    int isFirst = 1;

    if (output == NULL) {
        return;
    }

    if (root == NULL) {
        fprintf(output, "(empty)\n");
        return;
    }

    preorderRecursive(root, output, &isFirst);
    fputc('\n', output);
}

void inorder(TreeNode *root, FILE *output) {
    int isFirst = 1;

    if (output == NULL) {
        return;
    }

    if (root == NULL) {
        fprintf(output, "(empty)\n");
        return;
    }

    inorderRecursive(root, output, &isFirst);
    fputc('\n', output);
}

void postorder(TreeNode *root, FILE *output) {
    int isFirst = 1;

    if (output == NULL) {
        return;
    }

    if (root == NULL) {
        fprintf(output, "(empty)\n");
        return;
    }

    postorderRecursive(root, output, &isFirst);
    fputc('\n', output);
}

/*
 * Recursive helper for displayTree.
 * `hasVerticalBar[i]` records whether ancestor level `i` still has a sibling
 * below it, keeping indentation and ASCII branch connectors aligned.
 */
static void displayTreeRecursive(
    TreeNode *node,
    FILE *output,
    int depth,
    int isLeft,
    unsigned char *hasVerticalBar,
    int guideCapacity
) {
    int i;

    for (i = 1; i < depth; i++) {
        if (i < guideCapacity && hasVerticalBar[i]) {
            fputs("|   ", output);
        } else {
            fputs("    ", output);
        }
    }

    fprintf(output, "%s [%s] ", isLeft ? "|--" : "`--", isLeft ? "L" : "R");

    if (node == NULL) {
        fputs("#\n", output);
        return;
    }

    fprintf(output, "%s\n", node->label);

    /* Show both branches whenever at least one child exists so missing siblings are explicit. */
    if (node->left != NULL || node->right != NULL) {
        if (depth < guideCapacity) {
            hasVerticalBar[depth] = (unsigned char)isLeft;
        }
        displayTreeRecursive(node->left, output, depth + 1, 1, hasVerticalBar, guideCapacity);
        displayTreeRecursive(node->right, output, depth + 1, 0, hasVerticalBar, guideCapacity);
    }
}

void displayTree(TreeNode *root, FILE *output) {
    unsigned char *hasVerticalBar;
    int guideCapacity = MAX_NODES;

    if (output == NULL) {
        return;
    }

    if (root == NULL) {
        fprintf(output, "(empty tree)\n");
        return;
    }

    hasVerticalBar = (unsigned char *)calloc((size_t)guideCapacity, sizeof(unsigned char));
    if (hasVerticalBar == NULL) {
        fprintf(output, "[Root] %s (display truncated: out of memory)\n", root->label);
        return;
    }

    fprintf(output, "[Root] %s\n", root->label);

    if (root->left != NULL || root->right != NULL) {
        displayTreeRecursive(root->left, output, 1, 1, hasVerticalBar, guideCapacity);
        displayTreeRecursive(root->right, output, 1, 0, hasVerticalBar, guideCapacity);
    }

    free(hasVerticalBar);
}

/* Recursively frees child subtrees before freeing the parent node. */
void freeTree(TreeNode *root) {
    if (root == NULL) {
        return;
    }

    freeTree(root->left);
    freeTree(root->right);
    free(root);
}

/* Recursively checks label equality and left/right structural correspondence. */
int compareTrees(TreeNode *tree1, TreeNode *tree2) {
    if (tree1 == NULL && tree2 == NULL) {
        return 1;
    }

    if (tree1 == NULL || tree2 == NULL) {
        return 0;
    }

    if (strcmp(tree1->label, tree2->label) != 0) {
        return 0;
    }

    return compareTrees(tree1->left, tree2->left) &&
           compareTrees(tree1->right, tree2->right);
}

/* Recursively counts nodes: 1 (self) + left subtree + right subtree. */
int countNodes(TreeNode *root) {
    if (root == NULL) {
        return 0;
    }

    return 1 + countNodes(root->left) + countNodes(root->right);
}
