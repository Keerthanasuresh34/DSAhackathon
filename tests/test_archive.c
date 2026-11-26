/*
 * test_archive.c — Unit & Integration Tests for Member 2's Archive Module
 * Compiled directly against Member 1's backend/tree.h and backend/tree.c.
 */

#include "archive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_testsRun = 0;
static int g_testsFailed = 0;

#define TEST_ASSERT(cond, msg)                                                     \
    do {                                                                           \
        if (!(cond)) {                                                             \
            fprintf(stderr, "  [FAIL] %s:%d: %s\n", __FILE__, __LINE__, (msg));    \
            return 0;                                                              \
        }                                                                          \
    } while (0)

static void runTest(const char *name, int (*fn)(void)) {
    g_testsRun++;
    printf("[RUN ] %s\n", name);
    if (fn()) {
        printf("[PASS] %s\n", name);
    } else {
        g_testsFailed++;
        printf("[FAIL] %s\n", name);
    }
}

/* Captures Member 1's preorder() or inorder() output via a temporary file. */
static void captureTraversal(
    void (*fn)(TreeNode *, FILE *),
    TreeNode *root,
    char *buffer,
    size_t bufferSize
) {
    FILE *tmp;
    size_t n;

    if (buffer == NULL || bufferSize == 0) {
        return;
    }
    buffer[0] = '\0';

    tmp = tmpfile();
    if (tmp == NULL) {
        return;
    }

    fn(root, tmp);
    rewind(tmp);
    n = fread(buffer, 1, bufferSize - 1, tmp);
    buffer[n] = '\0';
    fclose(tmp);
}

static TreeNode *buildSampleFamilyTree(void) {
    TreeNode *a = createNode("A");
    TreeNode *b = createNode("B");
    TreeNode *c = createNode("C");
    TreeNode *d = createNode("D");

    a->left = b;
    a->right = c;
    b->right = d;
    return a;
}

static TreeNode *buildBalancedTreeHelper(int index, int count) {
    char label[MAX_LABEL_LENGTH];
    TreeNode *node;

    if (index > count) {
        return NULL;
    }

    snprintf(label, sizeof(label), "N%d", index);
    node = createNode(label);
    if (node == NULL) {
        return NULL;
    }

    node->left = buildBalancedTreeHelper(index * 2, count);
    node->right = buildBalancedTreeHelper(index * 2 + 1, count);
    return node;
}

static TreeNode *buildSkewedTree(int count, int goLeft) {
    TreeNode *root = NULL;
    TreeNode *curr = NULL;
    int i;

    for (i = 1; i <= count; i++) {
        char label[MAX_LABEL_LENGTH];
        TreeNode *node;
        snprintf(label, sizeof(label), "S%d", i);
        node = createNode(label);
        if (node == NULL) {
            freeTree(root);
            return NULL;
        }
        if (root == NULL) {
            root = node;
            curr = node;
        } else if (goLeft) {
            curr->left = node;
            curr = node;
        } else {
            curr->right = node;
            curr = node;
        }
    }
    return root;
}

/* -------------------------------------------------------------------------- */
/* Valid Tree & Round-Trip Tests                                              */
/* -------------------------------------------------------------------------- */

static int test_empty_tree(void) {
    char err[256];
    char *text = serializeTree(NULL);
    TreeNode *restored;

    TEST_ASSERT(text != NULL, "serializeTree(NULL) returned NULL");
    TEST_ASSERT(strcmp(text, "#") == 0, "Empty tree should serialize to '#'");
    TEST_ASSERT(validateArchive(text, err, sizeof(err)) == 1, "validateArchive('#') failed");

    restored = restoreTreeFromText(text, err, sizeof(err));
    TEST_ASSERT(restored == NULL, "Restored empty tree must be NULL");
    TEST_ASSERT(err[0] == '\0', "No error expected for valid empty tree");

    free(text);
    return 1;
}

static int test_single_node_tree(void) {
    char err[256];
    TreeNode *orig = createNode("Root");
    char *text = serializeTree(orig);
    TreeNode *restored;

    TEST_ASSERT(text != NULL, "serializeTree failed");
    TEST_ASSERT(strcmp(text, "Root # #") == 0, "Expected 'Root # #'");
    TEST_ASSERT(validateArchive(text, err, sizeof(err)) == 1, "Validation failed");

    restored = restoreTreeFromText(text, err, sizeof(err));
    TEST_ASSERT(restored != NULL, "Restoration failed");
    TEST_ASSERT(compareTrees(orig, restored) == 1, "Restored single node mismatch");

    free(text);
    freeTree(orig);
    freeTree(restored);
    return 1;
}

static int test_complete_tree(void) {
    char err[256];
    TreeNode *orig = buildBalancedTreeHelper(1, 7);
    char *text = serializeTree(orig);
    TreeNode *restored;

    TEST_ASSERT(text != NULL, "serializeTree failed");
    TEST_ASSERT(
        strcmp(text, "N1 N2 N4 # # N5 # # N3 N6 # # N7 # #") == 0,
        "Unexpected complete tree serialization"
    );

    restored = restoreTreeFromText(text, err, sizeof(err));
    TEST_ASSERT(restored != NULL, "Restoration failed");
    TEST_ASSERT(compareTrees(orig, restored) == 1, "Complete tree mismatch");

    free(text);
    freeTree(orig);
    freeTree(restored);
    return 1;
}

static int test_left_only_and_right_only(void) {
    char err[256];
    TreeNode *leftOnly = buildSkewedTree(3, 1);
    TreeNode *rightOnly = buildSkewedTree(3, 0);
    char *leftText = serializeTree(leftOnly);
    char *rightText = serializeTree(rightOnly);
    TreeNode *restoredLeft;
    TreeNode *restoredRight;

    TEST_ASSERT(leftText != NULL && rightText != NULL, "Serialization failed");
    TEST_ASSERT(strcmp(leftText, "S1 S2 S3 # # # #") == 0, "Left-only mismatch");
    TEST_ASSERT(strcmp(rightText, "S1 # S2 # S3 # #") == 0, "Right-only mismatch");
    TEST_ASSERT(strcmp(leftText, rightText) != 0, "Left-only and right-only must differ");

    restoredLeft = restoreTreeFromText(leftText, err, sizeof(err));
    restoredRight = restoreTreeFromText(rightText, err, sizeof(err));
    TEST_ASSERT(restoredLeft != NULL && restoredRight != NULL, "Restore failed");
    TEST_ASSERT(compareTrees(leftOnly, restoredLeft) == 1, "Left-only restore mismatch");
    TEST_ASSERT(compareTrees(rightOnly, restoredRight) == 1, "Right-only restore mismatch");
    TEST_ASSERT(compareTrees(restoredLeft, restoredRight) == 0, "Restored shapes collided");

    free(leftText);
    free(rightText);
    freeTree(leftOnly);
    freeTree(rightOnly);
    freeTree(restoredLeft);
    freeTree(restoredRight);
    return 1;
}

static int test_asymmetric_missing_children(void) {
    char err[256];
    TreeNode *missLeft = createNode("A");
    TreeNode *missRight = createNode("A");
    char *t1;
    char *t2;
    TreeNode *r1;
    TreeNode *r2;

    missLeft->right = createNode("B");
    missRight->left = createNode("B");

    t1 = serializeTree(missLeft);
    t2 = serializeTree(missRight);
    TEST_ASSERT(strcmp(t1, "A # B # #") == 0, "Expected 'A # B # #'");
    TEST_ASSERT(strcmp(t2, "A B # # #") == 0, "Expected 'A B # # #'");

    r1 = restoreTreeFromText(t1, err, sizeof(err));
    r2 = restoreTreeFromText(t2, err, sizeof(err));
    TEST_ASSERT(compareTrees(missLeft, r1) == 1, "Missing-left tree mismatch");
    TEST_ASSERT(compareTrees(missRight, r2) == 1, "Missing-right tree mismatch");
    TEST_ASSERT(compareTrees(r1, r2) == 0, "Left vs right child confused");

    free(t1);
    free(t2);
    freeTree(missLeft);
    freeTree(missRight);
    freeTree(r1);
    freeTree(r2);
    return 1;
}

static int test_round_trip_file_and_traversals(void) {
    const char *archivePath = "test_family_archive.txt";
    char err[256];
    char origPreorder[512];
    char origInorder[512];
    char restoredPreorder[512];
    char restoredInorder[512];

    TreeNode *orig = buildSampleFamilyTree();
    TreeNode *expected = buildSampleFamilyTree();
    TreeNode *restored;
    char *serialized;

    captureTraversal(preorder, orig, origPreorder, sizeof(origPreorder));
    captureTraversal(inorder, orig, origInorder, sizeof(origInorder));

    serialized = serializeTree(orig);
    TEST_ASSERT(serialized != NULL, "serializeTree failed");
    TEST_ASSERT(
        strcmp(serialized, "A B # D # # C # #") == 0,
        "Sample tree serialization did not match spec"
    );

    TEST_ASSERT(saveTreeToFile(orig, archivePath) == 1, "saveTreeToFile failed");

    freeTree(orig);
    orig = NULL;

    restored = restoreTreeFromFile(archivePath, err, sizeof(err));
    TEST_ASSERT(restored != NULL, "restoreTreeFromFile failed");
    TEST_ASSERT(countNodes(restored) == 4, "Restored node count mismatch");
    TEST_ASSERT(
        compareTrees(expected, restored) == 1,
        "Restored tree differs from expected structure"
    );

    captureTraversal(preorder, restored, restoredPreorder, sizeof(restoredPreorder));
    captureTraversal(inorder, restored, restoredInorder, sizeof(restoredInorder));
    TEST_ASSERT(
        strcmp(origPreorder, restoredPreorder) == 0,
        "Preorder traversal mismatch after round-trip"
    );
    TEST_ASSERT(
        strcmp(origInorder, restoredInorder) == 0,
        "Inorder traversal mismatch after round-trip"
    );

    free(serialized);
    freeTree(expected);
    freeTree(restored);
    remove(archivePath);
    return 1;
}

static int test_max_allowed_nodes_balanced_and_skewed(void) {
    char err[256];
    TreeNode *balanced = buildBalancedTreeHelper(1, MAX_ARCHIVE_NODES);
    TreeNode *skewed = buildSkewedTree(MAX_ARCHIVE_NODES, 1);
    char *balText;
    char *skewText;
    TreeNode *restoredBal;
    TreeNode *restoredSkew;

    TEST_ASSERT(balanced != NULL && skewed != NULL, "Failed to allocate 2000-node trees");

    balText = serializeTree(balanced);
    skewText = serializeTree(skewed);
    TEST_ASSERT(balText != NULL, "Failed to serialize 2000-node balanced tree");
    TEST_ASSERT(skewText != NULL, "Failed to serialize 2000-node skewed tree");

    TEST_ASSERT(validateArchive(balText, err, sizeof(err)) == 1, "2000 balanced failed validation");
    TEST_ASSERT(validateArchive(skewText, err, sizeof(err)) == 1, "2000 skewed failed validation");

    restoredBal = restoreTreeFromText(balText, err, sizeof(err));
    restoredSkew = restoreTreeFromText(skewText, err, sizeof(err));
    TEST_ASSERT(restoredBal != NULL && restoredSkew != NULL, "Failed to restore 2000-node trees");
    TEST_ASSERT(compareTrees(balanced, restoredBal) == 1, "2000 balanced mismatch");
    TEST_ASSERT(compareTrees(skewed, restoredSkew) == 1, "2000 skewed mismatch");

    free(balText);
    free(skewText);
    freeTree(balanced);
    freeTree(skewed);
    freeTree(restoredBal);
    freeTree(restoredSkew);
    return 1;
}

static int test_whitespace_and_max_label_boundary(void) {
    char err[256];
    const char *label31 = "Ancestor_1234567890123456789012";
    char archive[256];
    TreeNode *restored;

    TEST_ASSERT(strlen(label31) == 31, "Test setup error: expected 31 chars");
    snprintf(archive, sizeof(archive), " \n\t %s \r\n  # \t  #  \n ", label31);

    TEST_ASSERT(validateArchive(archive, err, sizeof(err)) == 1, "Valid 31-char archive rejected");
    restored = restoreTreeFromText(archive, err, sizeof(err));
    TEST_ASSERT(restored != NULL, "Failed to restore whitespace-padded archive");
    TEST_ASSERT(strcmp(restored->label, label31) == 0, "31-char label corrupted");
    TEST_ASSERT(restored->left == NULL && restored->right == NULL, "Children should be NULL");

    freeTree(restored);
    return 1;
}

/* -------------------------------------------------------------------------- */
/* Damaged / Invalid Archive Tests                                            */
/* -------------------------------------------------------------------------- */

static int expectInvalidText(const char *badText, const char *expectedSubstring) {
    char err[256];
    TreeNode *restored;

    TEST_ASSERT(
        validateArchive(badText, err, sizeof(err)) == 0,
        "validateArchive unexpectedly accepted invalid archive"
    );
    TEST_ASSERT(err[0] != '\0', "Expected non-empty error message from validateArchive");
    if (expectedSubstring != NULL) {
        TEST_ASSERT(
            strstr(err, expectedSubstring) != NULL,
            "Error message missing expected explanation"
        );
    }

    restored = restoreTreeFromText(badText, err, sizeof(err));
    TEST_ASSERT(restored == NULL, "restoreTreeFromText must return NULL on invalid input");
    TEST_ASSERT(err[0] != '\0', "Expected non-empty error message from restoreTreeFromText");
    return 1;
}

static int test_incomplete_archives(void) {
    TEST_ASSERT(expectInvalidText("A B #", "Incomplete archive"), "A B # failed");
    TEST_ASSERT(expectInvalidText("A B # X # #", "Incomplete archive"), "A B # X # # failed");
    TEST_ASSERT(expectInvalidText("A", "Incomplete archive"), "Single token A failed");
    return 1;
}

static int test_extra_tokens(void) {
    TEST_ASSERT(expectInvalidText("A # # C", "Extra tokens"), "A # # C failed");
    TEST_ASSERT(expectInvalidText("# #", "Extra tokens"), "# # failed");
    return 1;
}

static int test_empty_and_null_inputs(void) {
    TEST_ASSERT(expectInvalidText("", "Missing archive data"), "Empty string failed");
    TEST_ASSERT(expectInvalidText("   \t\r\n  ", "Missing archive data"), "Whitespace failed");
    TEST_ASSERT(expectInvalidText(NULL, "Missing archive data"), "NULL input failed");
    return 1;
}

static int test_invalid_labels(void) {
    char err[256];
    TreeNode *badNode;
    char *serialized;

    TEST_ASSERT(
        expectInvalidText("12345678901234567890123456789012 # #", "exceeds maximum length"),
        "32-char label not rejected"
    );

    TEST_ASSERT(expectInvalidText("A#B # #", "reserved"), "A#B not rejected");
    TEST_ASSERT(expectInvalidText("## # #", "reserved"), "## not rejected");
    TEST_ASSERT(expectInvalidText("A\x01B # #", "non-printable"), "Control char not rejected");

    /* Construct an unvalidated TreeNode to verify serializeTree rejects bad labels */
    badNode = (TreeNode *)malloc(sizeof(TreeNode));
    TEST_ASSERT(badNode != NULL, "malloc failed");
    strcpy(badNode->label, "#");
    badNode->left = NULL;
    badNode->right = NULL;
    serialized = serializeTree(badNode);
    TEST_ASSERT(serialized == NULL, "serializeTree must reject '#' node label");
    free(badNode);

    badNode = createNode("Bad Label");
    TEST_ASSERT(badNode != NULL, "createNode failed");
    serialized = serializeTree(badNode);
    TEST_ASSERT(serialized == NULL, "serializeTree must reject label with space");
    freeTree(badNode);

    TEST_ASSERT(validateArchive("A # #", err, sizeof(err)) == 1, "Sanity check failed");
    return 1;
}

static int test_exceeds_node_and_size_limits(void) {
    char err[256];
    TreeNode *tooMany;
    char *oversizedText;
    FILE *fp;
    const char *bigFile = "test_oversized_archive.txt";
    int i;
    size_t offset = 0;
    size_t cap = 65536;
    char *manyNodesText = (char *)malloc(cap);

    TEST_ASSERT(manyNodesText != NULL, "malloc failed");

    for (i = 1; i <= MAX_ARCHIVE_NODES + 1; i++) {
        char chunk[32];
        int n = snprintf(chunk, sizeof(chunk), "N%d # ", i);
        if (offset + (size_t)n + 10 >= cap) {
            cap *= 2;
            manyNodesText = (char *)realloc(manyNodesText, cap);
            TEST_ASSERT(manyNodesText != NULL, "realloc failed");
        }
        memcpy(manyNodesText + offset, chunk, (size_t)n);
        offset += (size_t)n;
    }
    manyNodesText[offset++] = '#';
    manyNodesText[offset] = '\0';

    TEST_ASSERT(
        expectInvalidText(manyNodesText, "maximum node limit"),
        "2001-node archive was not rejected"
    );
    free(manyNodesText);

    tooMany = buildSkewedTree(MAX_ARCHIVE_NODES + 1, 0);
    TEST_ASSERT(tooMany != NULL, "Failed to build 2001-node tree");
    TEST_ASSERT(serializeTree(tooMany) == NULL, "serializeTree must reject >2000 nodes");
    TEST_ASSERT(saveTreeToFile(tooMany, "should_not_exist.txt") == 0, "saveTreeToFile should fail");
    freeTree(tooMany);

    oversizedText = (char *)malloc(MAX_ARCHIVE_SIZE + 10);
    TEST_ASSERT(oversizedText != NULL, "malloc failed");
    memset(oversizedText, ' ', MAX_ARCHIVE_SIZE + 5);
    oversizedText[MAX_ARCHIVE_SIZE + 5] = '#';
    oversizedText[MAX_ARCHIVE_SIZE + 6] = '\0';
    TEST_ASSERT(
        expectInvalidText(oversizedText, "exceeds maximum size"),
        "Oversized text was not rejected"
    );
    free(oversizedText);

    fp = fopen(bigFile, "w");
    TEST_ASSERT(fp != NULL, "Failed to create oversized test file");
    for (i = 0; i < MAX_ARCHIVE_SIZE + 5; i++) {
        fputc(' ', fp);
    }
    fputc('#', fp);
    fclose(fp);

    TEST_ASSERT(
        restoreTreeFromFile(bigFile, err, sizeof(err)) == NULL,
        "Oversized file was not rejected"
    );
    TEST_ASSERT(
        strstr(err, "exceeds maximum allowed size") != NULL,
        "Oversized file error message mismatch"
    );
    remove(bigFile);

    return 1;
}

static int test_empty_and_missing_files(void) {
    const char *emptyFile = "test_empty_archive.txt";
    char err[256];
    FILE *fp;
    TreeNode *restored;

    fp = fopen(emptyFile, "w");
    TEST_ASSERT(fp != NULL, "Could not create empty test file");
    fclose(fp);

    restored = restoreTreeFromFile(emptyFile, err, sizeof(err));
    TEST_ASSERT(restored == NULL, "Empty file must fail restoration");
    TEST_ASSERT(strstr(err, "Missing archive data") != NULL, "Expected empty file message");
    remove(emptyFile);

    restored = restoreTreeFromFile("definitely_missing_archive_12345.txt", err, sizeof(err));
    TEST_ASSERT(restored == NULL, "Missing file must fail restoration");
    TEST_ASSERT(strstr(err, "Unable to open archive file") != NULL, "Expected open error");

    restored = restoreTreeFromFile("", err, sizeof(err));
    TEST_ASSERT(restored == NULL, "Empty filename must fail");
    TEST_ASSERT(saveTreeToFile(NULL, "") == 0, "saveTreeToFile must reject empty filename");

    return 1;
}

int main(void) {
    printf("=== Member 2 Archive Module Test Suite ===\n");

    runTest("Empty tree (#)", test_empty_tree);
    runTest("Single-node tree", test_single_node_tree);
    runTest("Complete tree (7 nodes)", test_complete_tree);
    runTest("Left-only and right-only skewed trees", test_left_only_and_right_only);
    runTest("Asymmetric missing left vs missing right", test_asymmetric_missing_children);
    runTest("Round-trip file save/restore + compareTrees + traversals", test_round_trip_file_and_traversals);
    runTest("Maximum allowed nodes (2000 balanced & skewed)", test_max_allowed_nodes_balanced_and_skewed);
    runTest("Whitespace handling and 31-char label boundary", test_whitespace_and_max_label_boundary);

    runTest("Incomplete archives (A B #, A B # X # #, A)", test_incomplete_archives);
    runTest("Extra trailing tokens (A # # C, # #)", test_extra_tokens);
    runTest("Empty, whitespace, and NULL inputs", test_empty_and_null_inputs);
    runTest("Invalid labels (32 chars, #, spaces, control chars)", test_invalid_labels);
    runTest("Exceeding node limit (2001) and MAX_ARCHIVE_SIZE", test_exceeds_node_and_size_limits);
    runTest("Empty file, missing file, and invalid filenames", test_empty_and_missing_files);

    printf("\nSummary: %d/%d tests passed.\n", g_testsRun - g_testsFailed, g_testsRun);
    return (g_testsFailed == 0) ? 0 : 1;
}
