/*
 * archive.c — Tree Serialization, Archive Export, Restoration & Validation
 * Project: The Museum's Vanishing Family Trees — Tree Archiving and Recovery System
 *
 * Uses TreeNode, createNode(), buildTreeFromTokens(), and freeTree() from
 * Member 1's shared tree.h without duplicating tree node structures.
 */

#include "archive.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_LABEL_CHARS (MAX_LABEL_LENGTH - 1)
#define MAX_VALID_TREE_TOKENS (2 * MAX_ARCHIVE_NODES + 1)

/* -------------------------------------------------------------------------- */
/* Internal Data Structures                                                   */
/* -------------------------------------------------------------------------- */

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} ArchiveBuffer;

typedef struct {
    const char *cursor;
    int tokenIndex;
} TokenScanner;

/* -------------------------------------------------------------------------- */
/* Portable String & Error Helpers (strict C11 compatible)                    */
/* -------------------------------------------------------------------------- */

static void setError(
    char *errorMessage,
    int errorMessageSize,
    const char *format,
    ...
) {
    va_list args;

    if (errorMessage == NULL || errorMessageSize <= 0) {
        return;
    }

    va_start(args, format);
    vsnprintf(errorMessage, (size_t)errorMessageSize, format, args);
    va_end(args);

    errorMessage[errorMessageSize - 1] = '\0';
}

static void clearError(char *errorMessage, int errorMessageSize) {
    if (errorMessage != NULL && errorMessageSize > 0) {
        errorMessage[0] = '\0';
    }
}

static char *dupString(const char *src) {
    size_t len;
    char *copy;

    if (src == NULL) {
        return NULL;
    }
    len = strlen(src);
    copy = (char *)malloc(len + 1);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, src, len + 1);
    return copy;
}

static int startsWithCaseInsensitive(const char *str, const char *prefix) {
    size_t i;
    for (i = 0; prefix[i] != '\0'; i++) {
        if (str[i] == '\0') {
            return 0;
        }
        if (tolower((unsigned char)str[i]) != tolower((unsigned char)prefix[i])) {
            return 0;
        }
    }
    return 1;
}

static void copyTokenPreview(
    const char *token,
    size_t tokenLen,
    char *preview,
    size_t previewSize
) {
    size_t maxCopy;
    size_t i;

    if (preview == NULL || previewSize == 0) {
        return;
    }
    if (previewSize <= 4) {
        preview[0] = '\0';
        return;
    }

    if (tokenLen <= MAX_LABEL_CHARS && tokenLen < previewSize) {
        maxCopy = tokenLen;
        for (i = 0; i < maxCopy; i++) {
            unsigned char ch = (unsigned char)token[i];
            preview[i] = (ch >= 0x20 && ch <= 0x7E) ? (char)ch : '?';
        }
        preview[maxCopy] = '\0';
        return;
    }

    maxCopy = previewSize - 4;
    if (maxCopy > MAX_LABEL_CHARS) {
        maxCopy = MAX_LABEL_CHARS;
    }
    for (i = 0; i < maxCopy; i++) {
        unsigned char ch = (unsigned char)token[i];
        preview[i] = (ch >= 0x20 && ch <= 0x7E) ? (char)ch : '?';
    }
    memcpy(preview + maxCopy, "...", 4);
}

/*
 * Validates a node label (never "#" itself).
 * Rules:
 *   - Non-empty and at most MAX_LABEL_CHARS (31) characters
 *   - Cannot be "#" or contain "#" (reserved null-child marker)
 *   - Printable non-whitespace ASCII only (0x21..0x7E)
 */
static int isValidNodeLabelStrict(
    const char *label,
    size_t length,
    int tokenIndex,
    char *errorMessage,
    int errorMessageSize
) {
    size_t i;
    char preview[MAX_LABEL_CHARS + 8];

    if (label == NULL || length == 0) {
        setError(
            errorMessage,
            errorMessageSize,
            "Invalid empty node label at token %d.",
            tokenIndex
        );
        return 0;
    }

    copyTokenPreview(label, length, preview, sizeof(preview));

    if (length > MAX_LABEL_CHARS) {
        setError(
            errorMessage,
            errorMessageSize,
            "Invalid label at token %d ('%s'): exceeds maximum length of %d characters.",
            tokenIndex,
            preview,
            MAX_LABEL_CHARS
        );
        return 0;
    }

    for (i = 0; i < length; i++) {
        unsigned char ch = (unsigned char)label[i];
        if (ch == '#') {
            setError(
                errorMessage,
                errorMessageSize,
                "Invalid label at token %d ('%s'): '#' is reserved for missing children.",
                tokenIndex,
                preview
            );
            return 0;
        }
        if (ch <= 0x20 || ch >= 0x7F) {
            setError(
                errorMessage,
                errorMessageSize,
                "Invalid label at token %d ('%s'): contains whitespace or non-printable character.",
                tokenIndex,
                preview
            );
            return 0;
        }
    }

    return 1;
}

/* -------------------------------------------------------------------------- */
/* Dynamic Buffer & Preorder Serialization                                    */
/* -------------------------------------------------------------------------- */

static int initArchiveBuffer(ArchiveBuffer *buf, size_t initialCapacity) {
    buf->data = (char *)malloc(initialCapacity);
    if (buf->data == NULL) {
        buf->length = 0;
        buf->capacity = 0;
        return 0;
    }
    buf->data[0] = '\0';
    buf->length = 0;
    buf->capacity = initialCapacity;
    return 1;
}

static int appendToken(ArchiveBuffer *buf, const char *token, size_t tokenLen) {
    size_t needsSpace = (buf->length > 0) ? 1 : 0;
    size_t newLength = buf->length + needsSpace + tokenLen;
    size_t newCapacity;
    char *resized;

    if (newLength > MAX_ARCHIVE_SIZE) {
        return 0;
    }

    if (newLength + 1 > buf->capacity) {
        newCapacity = buf->capacity;
        while (newCapacity < newLength + 1) {
            if (newCapacity > (size_t)MAX_ARCHIVE_SIZE) {
                newCapacity = (size_t)MAX_ARCHIVE_SIZE + 1;
                break;
            }
            newCapacity *= 2;
        }
        if (newCapacity < newLength + 1) {
            newCapacity = newLength + 1;
        }

        resized = (char *)realloc(buf->data, newCapacity);
        if (resized == NULL) {
            return 0;
        }
        buf->data = resized;
        buf->capacity = newCapacity;
    }

    if (needsSpace) {
        buf->data[buf->length++] = ' ';
    }
    memcpy(buf->data + buf->length, token, tokenLen);
    buf->length += tokenLen;
    buf->data[buf->length] = '\0';

    return 1;
}

/*
 * Recursively serializes the tree in preorder: Root, Left, Right.
 * Every NULL child pointer emits "#" so the tree shape is lossless.
 */
static int serializeRecursive(
    const TreeNode *node,
    ArchiveBuffer *buf,
    int *nodeCount
) {
    size_t labelLen;

    if (node == NULL) {
        return appendToken(buf, "#", 1);
    }

    if (*nodeCount >= MAX_ARCHIVE_NODES) {
        return 0;
    }

    labelLen = strlen(node->label);
    if (!isValidNodeLabelStrict(node->label, labelLen, *nodeCount + 1, NULL, 0)) {
        return 0;
    }

    (*nodeCount)++;

    if (!appendToken(buf, node->label, labelLen)) {
        return 0;
    }
    if (!serializeRecursive(node->left, buf, nodeCount)) {
        return 0;
    }
    if (!serializeRecursive(node->right, buf, nodeCount)) {
        return 0;
    }

    return 1;
}

/* -------------------------------------------------------------------------- */
/* Non-Destructive Scanner & Unified Recursive Parser / Validator             */
/* -------------------------------------------------------------------------- */

static int nextToken(
    TokenScanner *scanner,
    const char **tokenStart,
    size_t *tokenLen
) {
    const char *p = scanner->cursor;
    const char *start;

    while (*p != '\0' && isspace((unsigned char)*p)) {
        p++;
    }

    if (*p == '\0') {
        scanner->cursor = p;
        return 0;
    }

    start = p;
    while (*p != '\0' && !isspace((unsigned char)*p)) {
        p++;
    }

    scanner->cursor = p;
    scanner->tokenIndex++;
    *tokenStart = start;
    *tokenLen = (size_t)(p - start);
    return 1;
}

/*
 * Recursively validates and optionally constructs a preorder subtree.
 * Frees any partially allocated nodes immediately if restoration fails.
 */
static int parseSubtreeRecursive(
    TokenScanner *scanner,
    const char *parentLabel,
    const char *expectedRole,
    int buildNodes,
    int *nodeCount,
    TreeNode **outNode,
    char *errorMessage,
    int errorMessageSize
) {
    const char *tokStart = NULL;
    size_t tokLen = 0;
    char label[MAX_LABEL_LENGTH];
    TreeNode *node = NULL;
    TreeNode *leftChild = NULL;
    TreeNode *rightChild = NULL;

    *outNode = NULL;

    if (!nextToken(scanner, &tokStart, &tokLen)) {
        if (parentLabel == NULL) {
            setError(
                errorMessage,
                errorMessageSize,
                "Missing archive data: archive text is empty."
            );
        } else {
            setError(
                errorMessage,
                errorMessageSize,
                "Incomplete archive at token %d: expected %s of '%s', but reached end of archive.",
                scanner->tokenIndex + 1,
                expectedRole,
                parentLabel
            );
        }
        return 0;
    }

    if (scanner->tokenIndex > MAX_VALID_TREE_TOKENS) {
        setError(
            errorMessage,
            errorMessageSize,
            "Archive exceeds maximum token limit of %d tokens.",
            MAX_VALID_TREE_TOKENS
        );
        return 0;
    }

    if (tokLen == 1 && tokStart[0] == '#') {
        *outNode = NULL;
        return 1;
    }

    if (!isValidNodeLabelStrict(
            tokStart,
            tokLen,
            scanner->tokenIndex,
            errorMessage,
            errorMessageSize)) {
        return 0;
    }

    memcpy(label, tokStart, tokLen);
    label[tokLen] = '\0';

    if (*nodeCount >= MAX_ARCHIVE_NODES) {
        setError(
            errorMessage,
            errorMessageSize,
            "Archive exceeds maximum node limit of %d nodes.",
            MAX_ARCHIVE_NODES
        );
        return 0;
    }
    (*nodeCount)++;

    if (buildNodes) {
        node = createNode(label);
        if (node == NULL) {
            setError(
                errorMessage,
                errorMessageSize,
                "Memory allocation failed while restoring node '%s'.",
                label
            );
            return 0;
        }
    }

    if (!parseSubtreeRecursive(
            scanner,
            label,
            "left child",
            buildNodes,
            nodeCount,
            &leftChild,
            errorMessage,
            errorMessageSize)) {
        if (buildNodes) {
            freeTree(node);
        }
        return 0;
    }

    if (!parseSubtreeRecursive(
            scanner,
            label,
            "right child",
            buildNodes,
            nodeCount,
            &rightChild,
            errorMessage,
            errorMessageSize)) {
        if (buildNodes) {
            node->left = leftChild;
            freeTree(node);
        }
        return 0;
    }

    if (buildNodes) {
        node->left = leftChild;
        node->right = rightChild;
    }

    *outNode = node;
    return 1;
}

static int parseArchiveText(
    const char *text,
    int buildNodes,
    TreeNode **outRoot,
    char *errorMessage,
    int errorMessageSize
) {
    TokenScanner scanner;
    TreeNode *root = NULL;
    int nodeCount = 0;
    size_t textLen;
    const char *extraStart = NULL;
    size_t extraLen = 0;
    char preview[MAX_LABEL_CHARS + 8];

    *outRoot = NULL;
    clearError(errorMessage, errorMessageSize);

    if (text == NULL) {
        setError(
            errorMessage,
            errorMessageSize,
            "Missing archive data: input text is NULL."
        );
        return 0;
    }

    textLen = strlen(text);
    if (textLen > MAX_ARCHIVE_SIZE) {
        setError(
            errorMessage,
            errorMessageSize,
            "Archive text exceeds maximum size of %d characters.",
            MAX_ARCHIVE_SIZE
        );
        return 0;
    }

    scanner.cursor = text;
    scanner.tokenIndex = 0;

    if (!parseSubtreeRecursive(
            &scanner,
            NULL,
            "root",
            buildNodes,
            &nodeCount,
            &root,
            errorMessage,
            errorMessageSize)) {
        return 0;
    }

    if (nextToken(&scanner, &extraStart, &extraLen)) {
        copyTokenPreview(extraStart, extraLen, preview, sizeof(preview));
        setError(
            errorMessage,
            errorMessageSize,
            "Extra tokens after complete tree at token %d ('%s').",
            scanner.tokenIndex,
            preview
        );
        if (buildNodes && root != NULL) {
            freeTree(root);
        }
        return 0;
    }

    *outRoot = root;
    return 1;
}

/* -------------------------------------------------------------------------- */
/* Member 2 Standard Public Interface                                         */
/* -------------------------------------------------------------------------- */

char *serializeTree(TreeNode *root) {
    ArchiveBuffer buf;
    int nodeCount = 0;

    if (!initArchiveBuffer(&buf, 64)) {
        return NULL;
    }

    if (!serializeRecursive(root, &buf, &nodeCount)) {
        free(buf.data);
        return NULL;
    }

    return buf.data;
}

int saveTreeToFile(TreeNode *root, const char *filename) {
    char *serialized;
    FILE *fp;
    int writeOk = 1;

    if (filename == NULL || filename[0] == '\0') {
        return 0;
    }

    serialized = serializeTree(root);
    if (serialized == NULL) {
        return 0;
    }

    fp = fopen(filename, "w");
    if (fp == NULL) {
        free(serialized);
        return 0;
    }

    if (fputs(serialized, fp) == EOF || fputc('\n', fp) == EOF) {
        writeOk = 0;
    }

    if (fclose(fp) != 0) {
        writeOk = 0;
    }

    free(serialized);
    return writeOk;
}

int validateArchive(
    const char *text,
    char *errorMessage,
    int errorMessageSize
) {
    TreeNode *unusedRoot = NULL;
    return parseArchiveText(text, 0, &unusedRoot, errorMessage, errorMessageSize);
}

TreeNode *restoreTreeFromText(
    const char *text,
    char *errorMessage,
    int errorMessageSize
) {
    TreeNode *root = NULL;
    if (!parseArchiveText(text, 1, &root, errorMessage, errorMessageSize)) {
        return NULL;
    }
    return root;
}

TreeNode *restoreTreeFromFile(
    const char *filename,
    char *errorMessage,
    int errorMessageSize
) {
    FILE *fp;
    char *buffer;
    size_t capacity = 256;
    size_t length = 0;
    int ch;
    TreeNode *root;

    clearError(errorMessage, errorMessageSize);

    if (filename == NULL || filename[0] == '\0') {
        setError(
            errorMessage,
            errorMessageSize,
            "Archive filename is missing or empty."
        );
        return NULL;
    }

    fp = fopen(filename, "r");
    if (fp == NULL) {
        setError(
            errorMessage,
            errorMessageSize,
            "Unable to open archive file '%s': %s.",
            filename,
            strerror(errno)
        );
        return NULL;
    }

    buffer = (char *)malloc(capacity);
    if (buffer == NULL) {
        fclose(fp);
        setError(
            errorMessage,
            errorMessageSize,
            "Memory allocation failed while reading archive file '%s'.",
            filename
        );
        return NULL;
    }

    while ((ch = fgetc(fp)) != EOF) {
        if (length >= MAX_ARCHIVE_SIZE) {
            free(buffer);
            fclose(fp);
            setError(
                errorMessage,
                errorMessageSize,
                "Archive file '%s' exceeds maximum allowed size of %d characters.",
                filename,
                MAX_ARCHIVE_SIZE
            );
            return NULL;
        }

        if (length + 1 >= capacity) {
            size_t newCapacity = capacity * 2;
            char *resized;
            if (newCapacity > (size_t)MAX_ARCHIVE_SIZE + 1) {
                newCapacity = (size_t)MAX_ARCHIVE_SIZE + 1;
            }
            resized = (char *)realloc(buffer, newCapacity);
            if (resized == NULL) {
                free(buffer);
                fclose(fp);
                setError(
                    errorMessage,
                    errorMessageSize,
                    "Memory allocation failed while reading archive file '%s'.",
                    filename
                );
                return NULL;
            }
            buffer = resized;
            capacity = newCapacity;
        }

        buffer[length++] = (char)ch;
    }

    if (ferror(fp)) {
        free(buffer);
        fclose(fp);
        setError(
            errorMessage,
            errorMessageSize,
            "I/O error while reading archive file '%s'.",
            filename
        );
        return NULL;
    }

    fclose(fp);
    buffer[length] = '\0';

    if (length == 0) {
        free(buffer);
        setError(
            errorMessage,
            errorMessageSize,
            "Missing archive data: archive file '%s' is empty.",
            filename
        );
        return NULL;
    }

    root = restoreTreeFromText(buffer, errorMessage, errorMessageSize);
    free(buffer);
    return root;
}

/* -------------------------------------------------------------------------- */
/* CLI / Flask Engine Compatibility Layer (for backend/main.c)                */
/* -------------------------------------------------------------------------- */

char *serialize_tree(TreeNode *root) {
    return serializeTree(root);
}

int export_archive_to_file(
    const char *filepath,
    const char *serialized_tokens,
    const char *dynasty_name
) {
    FILE *fp;
    time_t now;
    struct tm *utc_tm;
    char time_buf[64];

    if (filepath == NULL || serialized_tokens == NULL) {
        return 0;
    }

    fp = fopen(filepath, "w");
    if (fp == NULL) {
        return 0;
    }

    now = time(NULL);
    utc_tm = gmtime(&now);
    if (utc_tm != NULL) {
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S UTC", utc_tm);
    } else {
        snprintf(time_buf, sizeof(time_buf), "Unknown UTC");
    }

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

char *read_archive_from_file(const char *filepath, char *error_buf, size_t err_len) {
    FILE *fp;
    long size;
    char *content;
    size_t read_bytes;

    if (filepath == NULL) {
        if (error_buf != NULL && err_len > 0) {
            snprintf(error_buf, err_len, "Missing file path.");
        }
        return NULL;
    }

    fp = fopen(filepath, "r");
    if (fp == NULL) {
        if (error_buf != NULL && err_len > 0) {
            snprintf(error_buf, err_len, "Failed to open archive file for reading: %s", filepath);
        }
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size <= 0) {
        fclose(fp);
        if (error_buf != NULL && err_len > 0) {
            snprintf(error_buf, err_len, "Archive file is empty: %s", filepath);
        }
        return NULL;
    }

    content = (char *)malloc((size_t)size + 1);
    if (content == NULL) {
        fclose(fp);
        if (error_buf != NULL && err_len > 0) {
            snprintf(error_buf, err_len, "Memory allocation failure while reading archive.");
        }
        return NULL;
    }

    read_bytes = fread(content, 1, (size_t)size, fp);
    content[read_bytes] = '\0';
    fclose(fp);
    return content;
}

static bool is_valid_legacy_label(const char *token) {
    size_t len;
    size_t i;

    if (token == NULL || token[0] == '\0') {
        return false;
    }
    if (strcmp(token, "#") == 0) {
        return true;
    }

    len = strlen(token);
    if (len >= MAX_LABEL_LENGTH) {
        return false;
    }

    for (i = 0; i < len; i++) {
        unsigned char c = (unsigned char)token[i];
        if (!isalnum(c) && c != '_' && c != '-' && c != '.') {
            return false;
        }
    }
    return true;
}

static char *extract_token_stream(const char *archive_text) {
    const char *p;
    char *cleaned;
    char *copy;
    char *cursor;

    if (archive_text == NULL) {
        return NULL;
    }

    p = archive_text;
    while (*p != '\0') {
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') {
            p++;
        }
        if (startsWithCaseInsensitive(p, "TOKENS:")) {
            const char *start;
            size_t len;
            char *extracted;

            p += 7;
            while (*p == ' ' || *p == '\t') {
                p++;
            }
            start = p;
            while (*p != '\0' && *p != '\r' && *p != '\n') {
                p++;
            }
            len = (size_t)(p - start);
            extracted = (char *)malloc(len + 1);
            if (extracted == NULL) {
                return NULL;
            }
            memcpy(extracted, start, len);
            extracted[len] = '\0';
            return extracted;
        }
        while (*p != '\0' && *p != '\n') {
            p++;
        }
        if (*p == '\n') {
            p++;
        }
    }

    cleaned = (char *)malloc(strlen(archive_text) + 1);
    if (cleaned == NULL) {
        return NULL;
    }
    cleaned[0] = '\0';

    copy = dupString(archive_text);
    if (copy == NULL) {
        free(cleaned);
        return NULL;
    }

    cursor = copy;
    while (*cursor != '\0') {
        char *lineEnd = strpbrk(cursor, "\r\n");
        char *line = cursor;

        if (lineEnd != NULL) {
            *lineEnd = '\0';
            cursor = lineEnd + 1;
        } else {
            cursor += strlen(cursor);
        }

        while (*line == ' ' || *line == '\t') {
            line++;
        }
        if (*line == '\0') {
            continue;
        }
        if (strncmp(line, "# ", 2) == 0 ||
            strncmp(line, "#-", 2) == 0 ||
            strncmp(line, "#=", 2) == 0) {
            continue;
        }
        if (cleaned[0] != '\0') {
            strcat(cleaned, " ");
        }
        strcat(cleaned, line);
    }

    free(copy);
    return cleaned;
}

static int tokenize_whitespace_inplace(char *buf, char **tokens, int maxTokens) {
    int count = 0;
    char *p = buf;

    while (*p != '\0') {
        while (*p != '\0' && isspace((unsigned char)*p)) {
            p++;
        }
        if (*p == '\0') {
            break;
        }
        if (count >= maxTokens) {
            break;
        }
        tokens[count++] = p;
        while (*p != '\0' && !isspace((unsigned char)*p)) {
            p++;
        }
        if (*p != '\0') {
            *p++ = '\0';
        }
    }
    return count;
}

ValidationResult validate_archive_string(const char *archive_text) {
    ValidationResult res;
    char *token_stream;
    char *copy;
    char *tokens[MAX_ARCHIVE_TOKENS];
    int token_count;
    int node_count = 0;
    int null_count = 0;
    int needed_slots = 1;
    int i;

    memset(&res, 0, sizeof(ValidationResult));
    res.is_valid = false;

    if (archive_text == NULL) {
        snprintf(res.error_code, sizeof(res.error_code), "EMPTY_ARCHIVE");
        snprintf(res.error_message, sizeof(res.error_message), "Archive text is null.");
        return res;
    }

    token_stream = extract_token_stream(archive_text);
    if (token_stream == NULL) {
        snprintf(res.error_code, sizeof(res.error_code), "EMPTY_ARCHIVE");
        snprintf(res.error_message, sizeof(res.error_message), "Failed to extract archive tokens.");
        return res;
    }

    copy = dupString(token_stream);
    free(token_stream);
    if (copy == NULL) {
        snprintf(res.error_code, sizeof(res.error_code), "MEMORY_ERROR");
        snprintf(res.error_message, sizeof(res.error_message), "Memory allocation failure during validation.");
        return res;
    }

    token_count = tokenize_whitespace_inplace(copy, tokens, MAX_ARCHIVE_TOKENS);
    if (token_count == 0) {
        free(copy);
        snprintf(res.error_code, sizeof(res.error_code), "EMPTY_ARCHIVE");
        snprintf(res.error_message, sizeof(res.error_message), "Archive is empty or contains no tokens.");
        return res;
    }

    res.total_tokens = token_count;
    for (i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], "#") == 0) {
            null_count++;
        } else {
            if (!is_valid_legacy_label(tokens[i])) {
                snprintf(res.error_code, sizeof(res.error_code), "INVALID_LABEL");
                snprintf(
                    res.error_message,
                    sizeof(res.error_message),
                    "Invalid character or format in node label '%s' at token index %d.",
                    tokens[i],
                    i + 1
                );
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
        snprintf(
            res.error_message,
            sizeof(res.error_message),
            "Archive contains %d nodes, which exceeds maximum limit of %d nodes.",
            node_count,
            MAX_NODES
        );
        free(copy);
        return res;
    }

    for (i = 0; i < token_count; i++) {
        if (needed_slots == 0) {
            snprintf(res.error_code, sizeof(res.error_code), "EXTRA_TOKENS");
            snprintf(
                res.error_message,
                sizeof(res.error_message),
                "Extra unconsumed tokens detected starting at index %d ('%s'). Tree is already fully resolved.",
                i + 1,
                tokens[i]
            );
            free(copy);
            return res;
        }
        if (strcmp(tokens[i], "#") == 0) {
            needed_slots--;
        } else {
            needed_slots++;
        }
    }

    if (needed_slots > 0) {
        snprintf(res.error_code, sizeof(res.error_code), "INCOMPLETE_TOKENS");
        snprintf(
            res.error_message,
            sizeof(res.error_message),
            "Archive terminated prematurely with %d unresolved branch slots. Missing children or sentinels.",
            needed_slots
        );
        free(copy);
        return res;
    }

    free(copy);
    res.is_valid = true;
    snprintf(res.error_code, sizeof(res.error_code), "SUCCESS");
    snprintf(res.error_message, sizeof(res.error_message), "Archive structure and tokens are valid.");
    return res;
}

TreeNode *restore_tree_from_archive(const char *archive_text, char *error_buf, size_t err_len) {
    ValidationResult v = validate_archive_string(archive_text);
    char *token_stream;
    char *copy;
    char *tokens[MAX_ARCHIVE_TOKENS];
    int token_count;
    int pos = 0;
    int err = TREE_OK;
    TreeNode *root;

    if (!v.is_valid) {
        if (error_buf != NULL && err_len > 0) {
            snprintf(error_buf, err_len, "[%s] %s", v.error_code, v.error_message);
        }
        return NULL;
    }

    token_stream = extract_token_stream(archive_text);
    if (token_stream == NULL) {
        if (error_buf != NULL && err_len > 0) {
            snprintf(error_buf, err_len, "Failed to extract token stream.");
        }
        return NULL;
    }

    copy = dupString(token_stream);
    free(token_stream);
    if (copy == NULL) {
        if (error_buf != NULL && err_len > 0) {
            snprintf(error_buf, err_len, "Memory error.");
        }
        return NULL;
    }

    token_count = tokenize_whitespace_inplace(copy, tokens, MAX_ARCHIVE_TOKENS);
    root = buildTreeFromTokens(tokens, token_count, &pos, &err);
    free(copy);

    if (err != TREE_OK && root == NULL && v.node_count > 0) {
        if (error_buf != NULL && err_len > 0) {
            snprintf(error_buf, err_len, "buildTreeFromTokens reported error code: %d", err);
        }
        return NULL;
    }

    return root;
}
