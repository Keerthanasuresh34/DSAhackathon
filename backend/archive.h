#ifndef ARCHIVE_H
#define ARCHIVE_H

#include "tree.h"
#include <stdbool.h>
#include <stddef.h>

/* Shared archive limits (tree.h defines MAX_LABEL_LENGTH = 32 incl. '\0') */
#define MAX_ARCHIVE_SIZE   100000
#define MAX_ARCHIVE_NODES  MAX_NODES
#define MAX_ARCHIVE_TOKENS 10000
#define MAX_BUFFER_LEN     32768

/* -------------------------------------------------------------------------- */
/* Member 2 Standard Archive Interface Contract                               */
/* -------------------------------------------------------------------------- */

char *serializeTree(TreeNode *root);

int saveTreeToFile(
    TreeNode *root,
    const char *filename
);

TreeNode *restoreTreeFromText(
    const char *text,
    char *errorMessage,
    int errorMessageSize
);

TreeNode *restoreTreeFromFile(
    const char *filename,
    char *errorMessage,
    int errorMessageSize
);

int validateArchive(
    const char *text,
    char *errorMessage,
    int errorMessageSize
);

/* -------------------------------------------------------------------------- */
/* CLI / Flask Engine Compatibility Interface (used by backend/main.c)        */
/* -------------------------------------------------------------------------- */

typedef struct {
    bool is_valid;
    int node_count;
    int null_count;
    int total_tokens;
    char error_code[64];
    char error_message[256];
} ValidationResult;

char *serialize_tree(TreeNode *root);

int export_archive_to_file(
    const char *filepath,
    const char *serialized_tokens,
    const char *dynasty_name
);

char *read_archive_from_file(
    const char *filepath,
    char *error_buf,
    size_t err_len
);

ValidationResult validate_archive_string(const char *archive_text);

TreeNode *restore_tree_from_archive(
    const char *archive_text,
    char *error_buf,
    size_t err_len
);

#endif /* ARCHIVE_H */
