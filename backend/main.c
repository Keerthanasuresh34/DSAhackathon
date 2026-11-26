#include "tree.h"
#include "archive.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_json_escaped(const char* str) {
    if (!str) {
        printf("null");
        return;
    }
    putchar('"');
    for (const char* p = str; *p; p++) {
        switch (*p) {
            case '"':  printf("\\\""); break;
            case '\\': printf("\\\\"); break;
            case '\b': printf("\\b"); break;
            case '\f': printf("\\f"); break;
            case '\n': printf("\\n"); break;
            case '\r': printf("\\r"); break;
            case '\t': printf("\\t"); break;
            default:
                if ((unsigned char)*p < 32) {
                    printf("\\u%04x", (unsigned char)*p);
                } else {
                    putchar(*p);
                }
                break;
        }
    }
    putchar('"');
}

static void print_json_error(const char* code, const char* message) {
    printf("{\"success\":false,\"error_code\":");
    print_json_escaped(code);
    printf(",\"error\":");
    print_json_escaped(message);
    printf("}\n");
}

/* Captures traversal output written to FILE* into a buffer */
static void capture_traversal_output(void (*fn)(TreeNode*, FILE*), TreeNode* root, char* buffer, size_t max_len) {
    if (!root) {
        buffer[0] = '\0';
        return;
    }
    const char* tmpPath = "_traversal_output.tmp";
    FILE* tmp = fopen(tmpPath, "w+b");
    if (!tmp) {
        buffer[0] = '\0';
        return;
    }
    fn(root, tmp);
    rewind(tmp);
    size_t read_bytes = fread(buffer, 1, max_len - 1, tmp);
    buffer[read_bytes] = '\0';
    fclose(tmp);
    remove(tmpPath);

    /* Strip trailing whitespace/newlines */
    size_t len = strlen(buffer);
    while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r' || buffer[len - 1] == ' ')) {
        buffer[len - 1] = '\0';
        len--;
    }
    if (strcmp(buffer, "(empty)") == 0 || strcmp(buffer, "(empty tree)") == 0) {
        buffer[0] = '\0';
    }
}

static int calculate_tree_height(TreeNode* root) {
    if (!root) return 0;
    int lh = calculate_tree_height(root->left);
    int rh = calculate_tree_height(root->right);
    return 1 + (lh > rh ? lh : rh);
}

static void json_helper(TreeNode* root, const char* side, char* buffer, size_t max_len) {
    if (!root) return;

    char snippet[512];
    snprintf(snippet, sizeof(snippet), "{\"name\":\"%s\",\"side\":\"%s\"", root->label, side ? side : "root");
    size_t cur = strlen(buffer);
    strncat(buffer, snippet, max_len - cur - 1);

    if (root->left || root->right) {
        cur = strlen(buffer);
        strncat(buffer, ",\"children\":[", max_len - cur - 1);

        if (root->left) {
            json_helper(root->left, "left", buffer, max_len);
        } else {
            strncat(buffer, "{\"name\":\"#\",\"side\":\"left\",\"isEmpty\":true}", max_len - strlen(buffer) - 1);
        }

        strncat(buffer, ",", max_len - strlen(buffer) - 1);

        if (root->right) {
            json_helper(root->right, "right", buffer, max_len);
        } else {
            strncat(buffer, "{\"name\":\"#\",\"side\":\"right\",\"isEmpty\":true}", max_len - strlen(buffer) - 1);
        }

        strncat(buffer, "]", max_len - strlen(buffer) - 1);
    }

    strncat(buffer, "}", max_len - strlen(buffer) - 1);
}

static void tree_to_json_str(TreeNode* root, char* buffer, size_t max_len) {
    if (!buffer || max_len == 0) return;
    buffer[0] = '\0';
    if (!root) {
        snprintf(buffer, max_len, "null");
        return;
    }
    json_helper(root, "root", buffer, max_len);
}

static void handle_health(void) {
    printf("{\"success\":true,\"status\":\"healthy\",\"engine\":\"C11-TreeCore-Engine\",\"version\":\"1.0.0\",\"max_nodes\":%d}\n", MAX_NODES);
}

static void handle_create(const char* tokens_str) {
    if (!tokens_str || strlen(tokens_str) == 0) {
        print_json_error("EMPTY_TOKENS", "Tokens string is empty.");
        return;
    }

    ValidationResult val = validate_archive_string(tokens_str);
    if (!val.is_valid) {
        print_json_error(val.error_code, val.error_message);
        return;
    }

    char err_buf[256] = "";
    TreeNode* root = restore_tree_from_archive(tokens_str, err_buf, sizeof(err_buf));
    if (!root && strcmp(tokens_str, "#") != 0 && val.node_count > 0) {
        print_json_error("PARSE_ERROR", err_buf[0] ? err_buf : "Failed to parse tokens into tree.");
        return;
    }

    int node_count = countNodes(root);
    int height = calculate_tree_height(root);

    char preorder_str[MAX_BUFFER_LEN] = "";
    char inorder_str[MAX_BUFFER_LEN] = "";
    char postorder_str[MAX_BUFFER_LEN] = "";
    char json_tree[MAX_BUFFER_LEN] = "";
    char ascii_art[MAX_BUFFER_LEN] = "";

    capture_traversal_output(preorder, root, preorder_str, sizeof(preorder_str));
    capture_traversal_output(inorder, root, inorder_str, sizeof(inorder_str));
    capture_traversal_output(postorder, root, postorder_str, sizeof(postorder_str));
    capture_traversal_output(displayTree, root, ascii_art, sizeof(ascii_art));
    tree_to_json_str(root, json_tree, sizeof(json_tree));

    printf("{\"success\":true,\"node_count\":%d,\"height\":%d,\"preorder\":", node_count, height);
    print_json_escaped(preorder_str);
    printf(",\"inorder\":");
    print_json_escaped(inorder_str);
    printf(",\"postorder\":");
    print_json_escaped(postorder_str);
    printf(",\"tree_json\":%s", json_tree[0] ? json_tree : "null");
    printf(",\"ascii_art\":");
    print_json_escaped(ascii_art);
    printf("}\n");

    freeTree(root);
}

static void handle_serialize(const char* tokens_str) {
    if (!tokens_str || strlen(tokens_str) == 0) {
        print_json_error("EMPTY_TOKENS", "Tokens string is empty.");
        return;
    }

    ValidationResult val = validate_archive_string(tokens_str);
    if (!val.is_valid) {
        print_json_error(val.error_code, val.error_message);
        return;
    }

    char err_buf[256] = "";
    TreeNode* root = restore_tree_from_archive(tokens_str, err_buf, sizeof(err_buf));
    char* serialized = serialize_tree(root);

    printf("{\"success\":true,\"serialized\":");
    print_json_escaped(serialized ? serialized : "");
    printf(",\"node_count\":%d}\n", countNodes(root));

    if (serialized) free(serialized);
    freeTree(root);
}

static void handle_export(const char* tokens_str, const char* filepath, const char* dynasty_name) {
    if (!tokens_str || strlen(tokens_str) == 0) {
        print_json_error("EMPTY_TOKENS", "Tokens string is empty.");
        return;
    }
    if (!filepath || strlen(filepath) == 0) {
        print_json_error("MISSING_FILEPATH", "Export destination file path is required.");
        return;
    }

    ValidationResult val = validate_archive_string(tokens_str);
    if (!val.is_valid) {
        print_json_error(val.error_code, val.error_message);
        return;
    }

    char err_buf[256] = "";
    TreeNode* root = restore_tree_from_archive(tokens_str, err_buf, sizeof(err_buf));
    char* serialized = serialize_tree(root);

    int export_ok = export_archive_to_file(filepath, serialized ? serialized : "#", dynasty_name);
    if (!export_ok) {
        print_json_error("FILE_WRITE_ERROR", "Failed to write archival export file to disk.");
        if (serialized) free(serialized);
        freeTree(root);
        return;
    }

    char read_err[256] = "";
    char* full_content = read_archive_from_file(filepath, read_err, sizeof(read_err));

    printf("{\"success\":true,\"filepath\":");
    print_json_escaped(filepath);
    printf(",\"archive_text\":");
    print_json_escaped(full_content ? full_content : "");
    printf(",\"node_count\":%d}\n", countNodes(root));

    if (full_content) free(full_content);
    if (serialized) free(serialized);
    freeTree(root);
}

static void handle_restore(const char* archive_text) {
    if (!archive_text || strlen(archive_text) == 0) {
        print_json_error("EMPTY_ARCHIVE", "Archive content is empty.");
        return;
    }

    ValidationResult val = validate_archive_string(archive_text);
    if (!val.is_valid) {
        print_json_error(val.error_code, val.error_message);
        return;
    }

    char err_buf[256] = "";
    TreeNode* root = restore_tree_from_archive(archive_text, err_buf, sizeof(err_buf));
    if (!root && val.node_count > 0) {
        print_json_error("RESTORATION_ERROR", err_buf[0] ? err_buf : "Restoration algorithm failed.");
        return;
    }

    int node_count = countNodes(root);
    int height = calculate_tree_height(root);

    char preorder_str[MAX_BUFFER_LEN] = "";
    char inorder_str[MAX_BUFFER_LEN] = "";
    char postorder_str[MAX_BUFFER_LEN] = "";
    char json_tree[MAX_BUFFER_LEN] = "";
    char ascii_art[MAX_BUFFER_LEN] = "";

    capture_traversal_output(preorder, root, preorder_str, sizeof(preorder_str));
    capture_traversal_output(inorder, root, inorder_str, sizeof(inorder_str));
    capture_traversal_output(postorder, root, postorder_str, sizeof(postorder_str));
    capture_traversal_output(displayTree, root, ascii_art, sizeof(ascii_art));
    tree_to_json_str(root, json_tree, sizeof(json_tree));

    printf("{\"success\":true,\"status\":\"RESTORED_SUCCESSFULLY\",\"node_count\":%d,\"height\":%d,\"preorder\":",
           node_count, height);
    print_json_escaped(preorder_str);
    printf(",\"inorder\":");
    print_json_escaped(inorder_str);
    printf(",\"postorder\":");
    print_json_escaped(postorder_str);
    printf(",\"tree_json\":%s", json_tree[0] ? json_tree : "null");
    printf(",\"ascii_art\":");
    print_json_escaped(ascii_art);
    printf("}\n");

    freeTree(root);
}

static void handle_restore_file(const char* filepath) {
    char err_buf[256] = "";
    char* content = read_archive_from_file(filepath, err_buf, sizeof(err_buf));
    if (!content) {
        print_json_error("FILE_READ_ERROR", err_buf[0] ? err_buf : "Failed to read archival file.");
        return;
    }

    handle_restore(content);
    free(content);
}

static void handle_validate(const char* archive_text) {
    if (!archive_text || strlen(archive_text) == 0) {
        printf("{\"success\":true,\"valid\":false,\"error_code\":\"EMPTY_ARCHIVE\",\"error_message\":\"Archive is empty or contains no tokens.\",\"node_count\":0,\"null_count\":0,\"total_tokens\":0}\n");
        return;
    }

    ValidationResult val = validate_archive_string(archive_text);
    printf("{\"success\":true,\"valid\":%s,\"error_code\":", val.is_valid ? "true" : "false");
    print_json_escaped(val.error_code);
    printf(",\"error_message\":");
    print_json_escaped(val.error_message);
    printf(",\"node_count\":%d,\"null_count\":%d,\"total_tokens\":%d}\n",
           val.node_count, val.null_count, val.total_tokens);
}

static void handle_compare(const char* tokens1, const char* tokens2) {
    if (!tokens1 || !tokens2) {
        print_json_error("MISSING_ARGUMENTS", "Both original tokens and comparison archive text are required.");
        return;
    }

    ValidationResult v1 = validate_archive_string(tokens1);
    ValidationResult v2 = validate_archive_string(tokens2);

    if (!v1.is_valid) {
        print_json_error("ORIGINAL_TREE_INVALID", v1.error_message);
        return;
    }
    if (!v2.is_valid) {
        print_json_error("RESTORED_TREE_INVALID", v2.error_message);
        return;
    }

    char err1[256] = "", err2[256] = "";
    TreeNode* t1 = restore_tree_from_archive(tokens1, err1, sizeof(err1));
    TreeNode* t2 = restore_tree_from_archive(tokens2, err2, sizeof(err2));

    int structural_match = compareTrees(t1, t2);

    char pre1[MAX_BUFFER_LEN] = "", pre2[MAX_BUFFER_LEN] = "";
    char in1[MAX_BUFFER_LEN] = "", in2[MAX_BUFFER_LEN] = "";
    char post1[MAX_BUFFER_LEN] = "", post2[MAX_BUFFER_LEN] = "";

    capture_traversal_output(preorder, t1, pre1, sizeof(pre1));
    capture_traversal_output(preorder, t2, pre2, sizeof(pre2));
    capture_traversal_output(inorder, t1, in1, sizeof(in1));
    capture_traversal_output(inorder, t2, in2, sizeof(in2));
    capture_traversal_output(postorder, t1, post1, sizeof(post1));
    capture_traversal_output(postorder, t2, post2, sizeof(post2));

    int pre_match = (strcmp(pre1, pre2) == 0);
    int in_match = (strcmp(in1, in2) == 0);
    int post_match = (strcmp(post1, post2) == 0);

    bool overall = (structural_match && pre_match && in_match && post_match);

    printf("{\"success\":true,\"overall_result\":");
    print_json_escaped(overall ? "MATCH" : "MISMATCH");
    printf(",\"structural_match\":%s", structural_match ? "true" : "false");
    printf(",\"preorder_match\":%s", pre_match ? "true" : "false");
    printf(",\"inorder_match\":%s", in_match ? "true" : "false");
    printf(",\"postorder_match\":%s", post_match ? "true" : "false");
    printf(",\"tree1\":{\"node_count\":%d,\"preorder\":", countNodes(t1));
    print_json_escaped(pre1);
    printf(",\"inorder\":");
    print_json_escaped(in1);
    printf(",\"postorder\":");
    print_json_escaped(post1);
    printf("},\"tree2\":{\"node_count\":%d,\"preorder\":", countNodes(t2));
    print_json_escaped(pre2);
    printf(",\"inorder\":");
    print_json_escaped(in2);
    printf(",\"postorder\":");
    print_json_escaped(post2);
    printf("}}\n");

    freeTree(t1);
    freeTree(t2);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("{\"success\":false,\"error\":\"No command option specified. Use --health, --create, --serialize, --export, --restore, --restore-file, --validate, or --compare.\"}\n");
        return 1;
    }

    const char* cmd = argv[1];

    if (strcmp(cmd, "--health") == 0) {
        handle_health();
    } else if (strcmp(cmd, "--create") == 0) {
        if (argc < 3) {
            print_json_error("MISSING_ARGUMENT", "Usage: --create <tokens>");
            return 1;
        }
        handle_create(argv[2]);
    } else if (strcmp(cmd, "--serialize") == 0) {
        if (argc < 3) {
            print_json_error("MISSING_ARGUMENT", "Usage: --serialize <tokens>");
            return 1;
        }
        handle_serialize(argv[2]);
    } else if (strcmp(cmd, "--export") == 0) {
        if (argc < 4) {
            print_json_error("MISSING_ARGUMENT", "Usage: --export <tokens> <filepath> [dynasty_name]");
            return 1;
        }
        const char* dynasty = (argc >= 5) ? argv[4] : "Ancient Dynasty";
        handle_export(argv[2], argv[3], dynasty);
    } else if (strcmp(cmd, "--restore") == 0) {
        if (argc < 3) {
            print_json_error("MISSING_ARGUMENT", "Usage: --restore <archive_text>");
            return 1;
        }
        handle_restore(argv[2]);
    } else if (strcmp(cmd, "--restore-file") == 0) {
        if (argc < 3) {
            print_json_error("MISSING_ARGUMENT", "Usage: --restore-file <filepath>");
            return 1;
        }
        handle_restore_file(argv[2]);
    } else if (strcmp(cmd, "--validate") == 0) {
        if (argc < 3) {
            handle_validate("");
        } else {
            handle_validate(argv[2]);
        }
    } else if (strcmp(cmd, "--compare") == 0) {
        if (argc < 4) {
            print_json_error("MISSING_ARGUMENT", "Usage: --compare <tokens1> <tokens2>");
            return 1;
        }
        handle_compare(argv[2], argv[3]);
    } else {
        print_json_error("UNKNOWN_COMMAND", "Unrecognized command argument.");
        return 1;
    }

    return 0;
}
