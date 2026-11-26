#ifndef ARCHIVE_H
#define ARCHIVE_H

#include "tree.h"
#include <stdbool.h>

#define MAX_ARCHIVE_TOKENS 10000
#define MAX_BUFFER_LEN 32768

typedef struct {
    bool is_valid;
    int node_count;
    int null_count;
    int total_tokens;
    char error_code[64];
    char error_message[256];
} ValidationResult;

/* Serialization */
char* serialize_tree(TreeNode* root);

/* File Export & Import */
int export_archive_to_file(const char* filepath, const char* serialized_tokens, const char* dynasty_name);
char* read_archive_from_file(const char* filepath, char* error_buf, size_t err_len);

/* Archive Integrity & Damage Analysis */
ValidationResult validate_archive_string(const char* archive_text);

/* Restoration */
TreeNode* restore_tree_from_archive(const char* archive_text, char* error_buf, size_t err_len);

#endif /* ARCHIVE_H */
