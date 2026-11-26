#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../backend/tree.h"

/*
 * Captures what a traversal or display function writes to a FILE * stream
 * using a portable local scratch file (avoids Windows MSVCRT tmpfile() C:\ issue).
 */
static void captureOutput(void (*fn)(TreeNode *, FILE *), TreeNode *root, char *buffer, size_t bufferSize) {
    const char *tmpPath = "tree_test_capture.tmp";
    FILE *tmp = fopen(tmpPath, "w+b");
    size_t bytesRead;

    assert(tmp != NULL);
    fn(root, tmp);
    rewind(tmp);

    bytesRead = fread(buffer, 1, bufferSize - 1, tmp);
    buffer[bytesRead] = '\0';
    fclose(tmp);
    remove(tmpPath);
}

static void testEmptyTree(void) {
    char *tokens[] = {"#"};
    int pos = 0;
    int err = -1;
    char out[256];

    TreeNode *root = buildTreeFromTokens(tokens, 1, &pos, &err);
    assert(root == NULL);
    assert(err == TREE_OK);
    assert(pos == 1);
    assert(countNodes(root) == 0);

    captureOutput(preorder, root, out, sizeof(out));
    assert(strcmp(out, "(empty)\n") == 0);

    captureOutput(inorder, root, out, sizeof(out));
    assert(strcmp(out, "(empty)\n") == 0);

    captureOutput(postorder, root, out, sizeof(out));
    assert(strcmp(out, "(empty)\n") == 0);

    captureOutput(displayTree, root, out, sizeof(out));
    assert(strcmp(out, "(empty tree)\n") == 0);

    /* NULL safety checks */
    preorder(NULL, NULL);
    inorder(NULL, NULL);
    postorder(NULL, NULL);
    displayTree(NULL, NULL);
    freeTree(NULL);
    assert(compareTrees(NULL, NULL) == 1);

    printf("[PASS] testEmptyTree\n");
}

static void testSingleNodeTree(void) {
    char *tokens[] = {"Ancestor", "#", "#"};
    int pos = 0;
    int err = -1;
    char out[256];

    TreeNode *root = buildTreeFromTokens(tokens, 3, &pos, &err);
    assert(root != NULL);
    assert(err == TREE_OK);
    assert(pos == 3);
    assert(countNodes(root) == 1);
    assert(strcmp(root->label, "Ancestor") == 0);
    assert(root->left == NULL && root->right == NULL);

    captureOutput(preorder, root, out, sizeof(out));
    assert(strcmp(out, "Ancestor\n") == 0);

    captureOutput(inorder, root, out, sizeof(out));
    assert(strcmp(out, "Ancestor\n") == 0);

    captureOutput(postorder, root, out, sizeof(out));
    assert(strcmp(out, "Ancestor\n") == 0);

    captureOutput(displayTree, root, out, sizeof(out));
    assert(strcmp(out, "[Root] Ancestor\n") == 0);

    freeTree(root);
    printf("[PASS] testSingleNodeTree\n");
}

static void testBalancedTree(void) {
    char *tokens[] = {
        "Root",
        "L1", "L2", "#", "#", "R2", "#", "#",
        "R1", "L3", "#", "#", "R3", "#", "#"
    };
    int pos = 0;
    int err = -1;
    char out[512];

    TreeNode *root = buildTreeFromTokens(tokens, 15, &pos, &err);
    assert(root != NULL);
    assert(err == TREE_OK);
    assert(pos == 15);
    assert(countNodes(root) == 7);

    captureOutput(preorder, root, out, sizeof(out));
    assert(strcmp(out, "Root L1 L2 R2 R1 L3 R3\n") == 0);

    captureOutput(inorder, root, out, sizeof(out));
    assert(strcmp(out, "L2 L1 R2 Root L3 R1 R3\n") == 0);

    captureOutput(postorder, root, out, sizeof(out));
    assert(strcmp(out, "L2 R2 L1 L3 R3 R1 Root\n") == 0);

    freeTree(root);
    printf("[PASS] testBalancedTree\n");
}

static void testLeftOnlyChild(void) {
    char *tokens[] = {"A", "B", "C", "#", "#", "#", "#"};
    int pos = 0;
    int err = -1;
    char out[512];

    TreeNode *root = buildTreeFromTokens(tokens, 7, &pos, &err);
    assert(root != NULL);
    assert(err == TREE_OK);
    assert(countNodes(root) == 3);
    assert(root->right == NULL);
    assert(root->left != NULL && root->left->right == NULL);

    captureOutput(preorder, root, out, sizeof(out));
    assert(strcmp(out, "A B C\n") == 0);

    captureOutput(inorder, root, out, sizeof(out));
    assert(strcmp(out, "C B A\n") == 0);

    captureOutput(postorder, root, out, sizeof(out));
    assert(strcmp(out, "C B A\n") == 0);

    captureOutput(displayTree, root, out, sizeof(out));
    assert(strstr(out, "`-- [R] #") != NULL);

    freeTree(root);
    printf("[PASS] testLeftOnlyChild\n");
}

static void testRightOnlyChild(void) {
    char *tokens[] = {"A", "#", "B", "#", "C", "#", "#"};
    int pos = 0;
    int err = -1;
    char out[512];

    TreeNode *root = buildTreeFromTokens(tokens, 7, &pos, &err);
    assert(root != NULL);
    assert(err == TREE_OK);
    assert(countNodes(root) == 3);
    assert(root->left == NULL);
    assert(root->right != NULL && root->right->left == NULL);

    captureOutput(preorder, root, out, sizeof(out));
    assert(strcmp(out, "A B C\n") == 0);

    captureOutput(inorder, root, out, sizeof(out));
    assert(strcmp(out, "A B C\n") == 0);

    captureOutput(postorder, root, out, sizeof(out));
    assert(strcmp(out, "C B A\n") == 0);

    captureOutput(displayTree, root, out, sizeof(out));
    assert(strstr(out, "|-- [L] #") != NULL);

    freeTree(root);
    printf("[PASS] testRightOnlyChild\n");
}

static void testMissingLeftWithRightPresentExample(void) {
    /* Expected example from specification: A B # D # # C # # */
    char *tokens[] = {"A", "B", "#", "D", "#", "#", "C", "#", "#"};
    int pos = 0;
    int err = -1;
    char out[512];

    TreeNode *root = buildTreeFromTokens(tokens, 9, &pos, &err);
    assert(root != NULL);
    assert(err == TREE_OK);
    assert(pos == 9);
    assert(countNodes(root) == 4);

    assert(strcmp(root->label, "A") == 0);
    assert(root->left != NULL && strcmp(root->left->label, "B") == 0);
    assert(root->right != NULL && strcmp(root->right->label, "C") == 0);
    assert(root->left->left == NULL);
    assert(root->left->right != NULL && strcmp(root->left->right->label, "D") == 0);
    assert(root->left->right->left == NULL && root->left->right->right == NULL);
    assert(root->right->left == NULL && root->right->right == NULL);

    captureOutput(preorder, root, out, sizeof(out));
    assert(strcmp(out, "A B D C\n") == 0);

    captureOutput(inorder, root, out, sizeof(out));
    assert(strcmp(out, "B D A C\n") == 0);

    captureOutput(postorder, root, out, sizeof(out));
    assert(strcmp(out, "D B C A\n") == 0);

    captureOutput(displayTree, root, out, sizeof(out));
    assert(strcmp(
        out,
        "[Root] A\n"
        "|-- [L] B\n"
        "|   |-- [L] #\n"
        "|   `-- [R] D\n"
        "`-- [R] C\n"
    ) == 0);

    printf("\n--- Display Output for Expected Example (A B # D # # C # #) ---\n");
    displayTree(root, stdout);
    printf("---------------------------------------------------------------\n");

    freeTree(root);
    printf("[PASS] testMissingLeftWithRightPresentExample\n");
}

static void testMaxNodesLimit(void) {
    /*
     * A right-skewed chain of N nodes requires 2*N + 1 preorder tokens:
     * ["N0", "#", "N1", "#", ..., "N{k}", "#", "#"]
     */
    const int okNodes = MAX_NODES;
    const int okTokensCount = 2 * okNodes + 1;
    char **okTokens = (char **)malloc((size_t)okTokensCount * sizeof(char *));
    int i;
    int pos = 0;
    int err = -1;
    TreeNode *okTree;

    assert(okTokens != NULL);
    for (i = 0; i < okNodes; i++) {
        char *buf = (char *)malloc(16);
        assert(buf != NULL);
        sprintf(buf, "N%d", i);
        okTokens[2 * i] = buf;
        okTokens[2 * i + 1] = "#";
    }
    okTokens[2 * okNodes] = "#";

    okTree = buildTreeFromTokens(okTokens, okTokensCount, &pos, &err);
    assert(okTree != NULL);
    assert(err == TREE_OK);
    assert(pos == okTokensCount);
    assert(countNodes(okTree) == MAX_NODES);
    freeTree(okTree);

    /* Now test MAX_NODES + 1 (2,001 nodes) -> must fail cleanly without leaks. */
    {
        const int overNodes = MAX_NODES + 1;
        const int overTokensCount = 2 * overNodes + 1;
        char **overTokens = (char **)malloc((size_t)overTokensCount * sizeof(char *));
        TreeNode *overTree;

        assert(overTokens != NULL);
        for (i = 0; i < okNodes; i++) {
            overTokens[2 * i] = okTokens[2 * i];
            overTokens[2 * i + 1] = "#";
        }
        overTokens[2 * okNodes] = "OVERFLOW";
        overTokens[2 * okNodes + 1] = "#";
        overTokens[2 * okNodes + 2] = "#";

        pos = 0;
        err = TREE_OK;
        overTree = buildTreeFromTokens(overTokens, overTokensCount, &pos, &err);
        assert(overTree == NULL);
        assert(err == TREE_ERR_MAX_NODES);

        free(overTokens);
    }

    for (i = 0; i < okNodes; i++) {
        free(okTokens[2 * i]);
    }
    free(okTokens);

    printf("[PASS] testMaxNodesLimit\n");
}

static void testInvalidLabelsAndInputs(void) {
    char tooLong32[33];
    char maxValid31[32];
    TreeNode *node;
    int pos;
    int err;

    memset(tooLong32, 'X', 32);
    tooLong32[32] = '\0';

    memset(maxValid31, 'Y', 31);
    maxValid31[31] = '\0';

    /* Direct createNode checks */
    assert(createNode(NULL) == NULL);
    assert(createNode("") == NULL);
    assert(createNode("#") == NULL);
    assert(createNode(tooLong32) == NULL);

    node = createNode(maxValid31);
    assert(node != NULL);
    assert(strlen(node->label) == 31);
    freeTree(node);

    /* buildTreeFromTokens invalid inputs and mid-tree failures (no leaks) */
    {
        char *emptyInTree[] = {"Root", "", "#", "#", "#"};
        pos = 0;
        err = TREE_OK;
        assert(buildTreeFromTokens(emptyInTree, 5, &pos, &err) == NULL);
        assert(err == TREE_ERR_INVALID_LABEL);
    }
    {
        char *longInTree[] = {"Root", "Left", "#", "#", tooLong32, "#", "#"};
        pos = 0;
        err = TREE_OK;
        assert(buildTreeFromTokens(longInTree, 7, &pos, &err) == NULL);
        assert(err == TREE_ERR_INVALID_LABEL);
    }
    {
        char *nullElemTree[] = {"Root", NULL, "#", "#"};
        pos = 0;
        err = TREE_OK;
        assert(buildTreeFromTokens(nullElemTree, 4, &pos, &err) == NULL);
        assert(err == TREE_ERR_INVALID_LABEL);
    }
    {
        char *truncatedTree[] = {"Root", "Left", "#"};
        pos = 0;
        err = TREE_OK;
        assert(buildTreeFromTokens(truncatedTree, 3, &pos, &err) == NULL);
        assert(err == TREE_ERR_TRUNCATED);
    }
    {
        pos = 0;
        err = TREE_OK;
        assert(buildTreeFromTokens(NULL, 0, &pos, &err) == NULL);
        assert(err == TREE_ERR_INVALID_ARG);
    }
    {
        char *validOne[] = {"Solo", "#", "#"};
        pos = -1;
        err = TREE_OK;
        assert(buildTreeFromTokens(validOne, 3, &pos, &err) == NULL);
        assert(err == TREE_ERR_INVALID_ARG);

        /* Passing error == NULL must not crash */
        pos = 0;
        node = buildTreeFromTokens(validOne, 3, &pos, NULL);
        assert(node != NULL);
        freeTree(node);
    }

    printf("[PASS] testInvalidLabelsAndInputs\n");
}

static void testTreeComparison(void) {
    char *tokensA[] = {"A", "B", "#", "D", "#", "#", "C", "#", "#"};
    char *tokensSame[] = {"A", "B", "#", "D", "#", "#", "C", "#", "#"};
    char *tokensDiffLabel[] = {"A", "B", "#", "E", "#", "#", "C", "#", "#"};
    char *tokensSwappedBranch[] = {"A", "B", "D", "#", "#", "#", "C", "#", "#"};

    int pos = 0;
    int err = 0;

    TreeNode *t1 = buildTreeFromTokens(tokensA, 9, &pos, &err);
    TreeNode *t2 = (pos = 0, buildTreeFromTokens(tokensSame, 9, &pos, &err));
    TreeNode *t3 = (pos = 0, buildTreeFromTokens(tokensDiffLabel, 9, &pos, &err));
    TreeNode *t4 = (pos = 0, buildTreeFromTokens(tokensSwappedBranch, 9, &pos, &err));

    assert(t1 != NULL && t2 != NULL && t3 != NULL && t4 != NULL);

    assert(compareTrees(t1, t2) == 1);
    assert(compareTrees(t1, t3) == 0);
    assert(compareTrees(t1, t4) == 0);
    assert(compareTrees(t1, NULL) == 0);
    assert(compareTrees(NULL, t1) == 0);
    assert(compareTrees(NULL, NULL) == 1);

    freeTree(t1);
    freeTree(t2);
    freeTree(t3);
    freeTree(t4);

    printf("[PASS] testTreeComparison\n");
}

static void testMemoryCleanupStress(void) {
    int cycle;
    char *tokens[] = {"Founder", "ChildA", "GrandA", "#", "#", "#", "ChildB", "#", "GrandB", "#", "#"};

    for (cycle = 0; cycle < 500; cycle++) {
        int pos = 0;
        int err = 0;
        TreeNode *tree = buildTreeFromTokens(tokens, 11, &pos, &err);
        assert(tree != NULL);
        assert(err == TREE_OK);
        assert(countNodes(tree) == 5);
        freeTree(tree);
    }

    printf("[PASS] testMemoryCleanupStress\n");
}

int main(void) {
    printf("=== Binary Tree Core Module Verification Suite ===\n");

    testEmptyTree();
    testSingleNodeTree();
    testBalancedTree();
    testLeftOnlyChild();
    testRightOnlyChild();
    testMissingLeftWithRightPresentExample();
    testMaxNodesLimit();
    testInvalidLabelsAndInputs();
    testTreeComparison();
    testMemoryCleanupStress();

    printf("\nAll 10 test suites passed successfully!\n");
    return 0;
}
