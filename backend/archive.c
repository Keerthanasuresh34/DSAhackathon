#include "archive.h"
#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

static void serialize_helper(TreeNode* root, char* buffer, size_t max_len) {
    if (!root) {
        size_t cur = strlen(buffer);
        if (cur > 0 && cur < max_len - 2) {
            strncat(buffer, " ", max_len - cur - 1);
            cur++;
        }
        if (cur < max_len - 1) {
            strncat(buffer, "#", max_len - cur - 1);
        }
        return;
    }

    size_t cur = strlen(buffer);
    if (cur > 0 && cur < max_len - 2) {
        strncat(buffer, " ", max_len - cur - 1);
        cur++;
    }
    if (cur < max_len - 1) {
        strncat(buffer, root->label, max_len - cur - 1);
    }

    serialize_helper(root->left, buffer, max_len);
    serialize_helper(root->right, buffer, max_len);
}

char* serialize_tree(TreeNode* root) {
    char* buffer = (char*)malloc(MAX_BUFFER_LEN);
    if (!buffer) return NULL;
    buffer[0] = '\0';

    if (!root) {
        snprintf(buffer, MAX_BUFFER_LEN, "#");
        return buffer;
    }

    serialize_helper(root, buffer, MAX_BUFFER_LEN);
    return buffer;
}

int export_archive_to_file(const char* filepath, const char* serialized_tokens, const char* dynasty_name) {
    if (!filepath || !serialized_tokens) return 0;

    FILE* fp = fopen(filepath, "w");
    if (!fp) return 0;

    time_t now = time(NULL);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S UTC", gmtime(&now));

    fprintf(fp, "# ====================================================================\n");
    fprintf(fp, "# THE MUSEUM'S VANISHING FAMILY TREES — OFFICIAL ARCHIVAL RECORD\n");
    fprintf(fp, "# Artifact Registry: Ancient Royal Lineage Preservation System\n");
    fprintf(fp, "# Specification: PREORDER_NULL_SENTINEL_V1\n");
    fprintf(fp, "# Dynasty Designation: %s\n", dynasty_name ? dynasty_name : "Imperial Royal Ancestry");
    fprintf(fp, "# Archival Timestamp: %s\n", time_buf);
    fprintf(fp, "# ====================================================================\n\n");
    fprintf(fp, "TOKENS: %s\n\n", serialized_tokens);
    fprintf(fp, "# ====================================================================\n");
    fprintf(fp, "# END OF ARCHIVAL RECORD — INTEGRITY VERIFIED\n");
    fprintf(fp, "# ====================================================================\n");

    fclose(fp);
    return 1;
}

char* read_archive_from_file(const char* filepath, char* error_buf, size_t err_len) {
    if (!filepath) {
        if (error_buf) snprintf(error_buf, err_len, "Missing file path.");
        return NULL;
    }

    FILE* fp = fopen(filepath, "r");
    if (!fp) {
        if (error_buf) snprintf(error_buf, err_len, "Failed to open archive file for reading: %s", filepath);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size <= 0) {
        fclose(fp);
        if (error_buf) snprintf(error_buf, err_len, "Archive file is empty: %s", filepath);
        return NULL;
    }

    char* content = (char*)malloc(size + 1);
    if (!content) {
        fclose(fp);
        if (error_buf) snprintf(error_buf, err_len, "Memory allocation failure while reading archive.");
        return NULL;
    }

    size_t read_bytes = fread(content, 1, size, fp);
    content[read_bytes] = '\0';
    fclose(fp);
    return content;
}

static bool is_valid_node_label(const char* token) {
    if (!token || strlen(token) == 0) return false;
    if (strcmp(token, "#") == 0) return true;

    if (strlen(token) >= MAX_LABEL_LENGTH) return false;

    for (size_t i = 0; i < strlen(token); i++) {
        char c = token[i];
        if (!isalnum((unsigned char)c) && c != '_' && c != '-' && c != '.') {
            return false;
        }
    }
    return true;
}

static char* extract_token_stream(const char* archive_text) {
    if (!archive_text) return NULL;

    const char* p = archive_text;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        if (strncasecmp(p, "TOKENS:", 7) == 0) {
            p += 7;
            while (*p == ' ' || *p == '\t') p++;
            const char* start = p;
            while (*p && *p != '\r' && *p != '\n') p++;
            size_t len = p - start;
            char* extracted = (char*)malloc(len + 1);
            if (!extracted) return NULL;
            strncpy(extracted, start, len);
            extracted[len] = '\0';
            return extracted;
        }
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
    }

    char* cleaned = (char*)malloc(strlen(archive_text) + 1);
    if (!cleaned) return NULL;
    cleaned[0] = '\0';

    char* copy = strdup(archive_text);
    if (!copy) {
        free(cleaned);
        return NULL;
    }

    char* line = strtok(copy, "\r\n");
    while (line) {
        while (*line == ' ' || *line == '\t') line++;
        if (strncmp(line, "# ", 2) == 0 || strncmp(line, "#-", 2) == 0 || strncmp(line, "#=", 2) == 0) {
            /* Skip comment line */
        } else {
            if (strlen(cleaned) > 0) {
                strcat(cleaned, " ");
            }
            strcat(cleaned, line);
        }
        line = strtok(NULL, "\r\n");
    }

    free(copy);
    return cleaned;
}

ValidationResult validate_archive_string(const char* archive_text) {
    ValidationResult res;
    memset(&res, 0, sizeof(ValidationResult));
    res.is_valid = false;

    if (!archive_text) {
        snprintf(res.error_code, sizeof(res.error_code), "EMPTY_ARCHIVE");
        snprintf(res.error_message, sizeof(res.error_message), "Archive text is null.");
        return res;
    }

    char* token_stream = extract_token_stream(archive_text);
    if (!token_stream) {
        snprintf(res.error_code, sizeof(res.error_code), "EMPTY_ARCHIVE");
        snprintf(res.error_message, sizeof(res.error_message), "Failed to extract archive tokens.");
        return res;
    }

    char* copy = strdup(token_stream);
    free(token_stream);
    if (!copy) {
        snprintf(res.error_code, sizeof(res.error_code), "MEMORY_ERROR");
        snprintf(res.error_message, sizeof(res.error_message), "Memory allocation failure during validation.");
        return res;
    }

    char* tokens[MAX_ARCHIVE_TOKENS];
    int token_count = 0;
    char* saveptr = NULL;
    char* tok = strtok_r(copy, " \t\r\n", &saveptr);

    while (tok && token_count < MAX_ARCHIVE_TOKENS) {
        tokens[token_count++] = tok;
        tok = strtok_r(NULL, " \t\r\n", &saveptr);
    }

    if (token_count == 0) {
        free(copy);
        snprintf(res.error_code, sizeof(res.error_code), "EMPTY_ARCHIVE");
        snprintf(res.error_message, sizeof(res.error_message), "Archive is empty or contains no tokens.");
        return res;
    }

    res.total_tokens = token_count;
    int node_count = 0;
    int null_count = 0;

    for (int i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], "#") == 0) {
            null_count++;
        } else {
            if (!is_valid_node_label(tokens[i])) {
                snprintf(res.error_code, sizeof(res.error_code), "INVALID_LABEL");
                snprintf(res.error_message, sizeof(res.error_message),
                         "Invalid character or format in node label '%s' at token index %d.", tokens[i], i + 1);
                free(copy);
                return res;
            }
            node_count++;
        }
    }

    res.node_count = node_count;
    res.null_count = null_count;

    if (node_count > MAX_NODES) {
        snprintf(res.error_code, sizeof(res.error_code), "EXCEEDS_NODE_LIMIT");
        snprintf(res.error_message, sizeof(res.error_message),
                 "Archive contains %d nodes, which exceeds maximum limit of %d nodes.", node_count, MAX_NODES);
        free(copy);
        return res;
    }

    int needed_slots = 1;
    for (int i = 0; i < token_count; i++) {
        if (needed_slots == 0) {
            snprintf(res.error_code, sizeof(res.error_code), "EXTRA_TOKENS");
            snprintf(res.error_message, sizeof(res.error_message),
                     "Extra unconsumed tokens detected starting at index %d ('%s'). Tree is already fully resolved.",
                     i + 1, tokens[i]);
            free(copy);
            return res;
        }

        if (strcmp(tokens[i], "#") == 0) {
            needed_slots--;
        } else {
            needed_slots += 1;
        }
    }

    if (needed_slots > 0) {
        snprintf(res.error_code, sizeof(res.error_code), "INCOMPLETE_TOKENS");
        snprintf(res.error_message, sizeof(res.error_message),
                 "Archive terminated prematurely with %d unresolved branch slots. Missing children or sentinels.",
                 needed_slots);
        free(copy);
        return res;
    }

    free(copy);
    res.is_valid = true;
    snprintf(res.error_code, sizeof(res.error_code), "SUCCESS");
    snprintf(res.error_message, sizeof(res.error_message), "Archive structure and tokens are valid.");
    return res;
}

TreeNode* restore_tree_from_archive(const char* archive_text, char* error_buf, size_t err_len) {
    ValidationResult v = validate_archive_string(archive_text);
    if (!v.is_valid) {
        if (error_buf) {
            snprintf(error_buf, err_len, "[%s] %s", v.error_code, v.error_message);
        }
        return NULL;
    }

    char* token_stream = extract_token_stream(archive_text);
    if (!token_stream) {
        if (error_buf) snprintf(error_buf, err_len, "Failed to extract token stream.");
        return NULL;
    }

    char* copy = strdup(token_stream);
    free(token_stream);
    if (!copy) {
        if (error_buf) snprintf(error_buf, err_len, "Memory error.");
        return NULL;
    }

    char* tokens[MAX_ARCHIVE_TOKENS];
    int token_count = 0;
    char* saveptr = NULL;
    char* tok = strtok_r(copy, " \t\r\n", &saveptr);

    while (tok && token_count < MAX_ARCHIVE_TOKENS) {
        tokens[token_count++] = tok;
        tok = strtok_r(NULL, " \t\r\n", &saveptr);
    }

    int pos = 0;
    int err = TREE_OK;
    TreeNode* root = buildTreeFromTokens(tokens, token_count, &pos, &err);
    free(copy);

    if (err != TREE_OK && root == NULL && v.node_count > 0) {
        if (error_buf) {
            snprintf(error_buf, err_len, "buildTreeFromTokens reported error code: %d", err);
        }
        return NULL;
    }

    return root;
}
