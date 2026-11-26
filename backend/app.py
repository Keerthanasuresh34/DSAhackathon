"""
The Museum's Vanishing Family Trees — Flask Backend API Server
Developed in coordination with Member 4 (Backend API) and Member 3 (Frontend UI)

Provides REST endpoints for:
- Health check
- Preorder binary tree construction & verification
- Traversals (Preorder, Inorder, Postorder)
- Lossless serialization & archival file generation
- Recovery from archived manuscript
- Validation and rejection diagnostics for damaged/truncated streams
- Structural comparison & round-trip verification
- Static hosting of frontend dashboard
"""

import os
from flask import Flask, request, jsonify, send_from_directory

app = Flask(__name__, static_folder="../frontend")

# Enable CORS for local testing from any origin or port
@app.after_request
def add_cors_headers(response):
    response.headers["Access-Control-Allow-Origin"] = "*"
    response.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization"
    response.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS"
    return response

# Binary Tree Node Structure
class TreeNode:
    def __init__(self, val):
        self.val = val
        self.left = None
        self.right = None

    def to_dict(self):
        return {
            "val": self.val,
            "left": self.left.to_dict() if self.left else None,
            "right": self.right.to_dict() if self.right else None
        }

def build_tree_from_tokens(tokens):
    """
    Constructs a binary tree from preorder tokens with '#' representing NULL.
    Strictly verifies that:
    1. Stream contains enough tokens for complete reconstruction.
    2. Stream has no extraneous dangling tokens after full tree reconstruction.
    """
    if not tokens:
        raise ValueError("Empty manuscript: No tokens provided.")

    index = 0

    def helper(parent_val=None, branch=None):
        nonlocal index
        if index >= len(tokens):
            err_msg = (
                f"Premature stream termination: Node '{parent_val}' expects {branch} child, "
                f"but token stream ended abruptly."
                if parent_val else "Premature stream termination: Expected root token."
            )
            raise ValueError(err_msg)

        tok = tokens[index]
        index += 1

        if tok == "#":
            return None

        # Disallow unrecognized invalid symbol glyphs
        if any(c in tok for c in ["%", "^", "@", "$", "*", "!", "&"]):
            raise ValueError(f"Corrupted heraldic glyph '{tok}' is not a recognized lineage token.")

        node = TreeNode(tok)
        node.left = helper(tok, "left")
        node.right = helper(tok, "right")
        return node

    root = helper()

    # Check for excess unused tokens
    if index < len(tokens):
        extra = " ".join(tokens[index:])
        raise ValueError(
            f"Excess tokens detected: Tree structure completed, but dangling tokens remain: '{extra}'."
        )

    return root

def get_preorder(root):
    res = []
    def traverse(n):
        if not n:
            return
        res.append(n.val)
        traverse(n.left)
        traverse(n.right)
    traverse(root)
    return res

def get_inorder(root):
    res = []
    def traverse(n):
        if not n:
            return
        traverse(n.left)
        res.append(n.val)
        traverse(n.right)
    traverse(root)
    return res

def get_postorder(root):
    res = []
    def traverse(n):
        if not n:
            return
        traverse(n.left)
        traverse(n.right)
        res.append(n.val)
    traverse(root)
    return res

def get_tree_height(root):
    if not root:
        return 0
    return 1 + max(get_tree_height(root.left), get_tree_height(root.right))

def serialize_tree(root):
    tokens = []
    def traverse(n):
        if not n:
            tokens.append("#")
            return
        tokens.append(str(n.val))
        traverse(n.left)
        traverse(n.right)
    traverse(root)
    return " ".join(tokens)

# ================= API ENDPOINTS =================

@app.route("/", methods=["GET"])
def serve_index():
    return send_from_directory("../frontend", "index.html")

@app.route("/<path:path>", methods=["GET"])
def serve_static(path):
    return send_from_directory("../frontend", path)

@app.route("/api/health", methods=["GET"])
def health_check():
    return jsonify({
        "status": "ok",
        "backend": "C-BinaryTree-Engine",
        "version": "1.0.0",
        "institution": "Museum Archives & Lineage Conservancy"
    })

@app.route("/api/tree/create", methods=["POST", "OPTIONS"])
def create_tree():
    if request.method == "OPTIONS":
        return "", 200

    data = request.get_json(silent=True) or {}
    tokens_str = data.get("tokens", "").strip()

    if not tokens_str:
        return jsonify({"success": False, "error": "Empty input: Inscribe preorder tokens."}), 400

    tokens = tokens_str.split()
    try:
        root = build_tree_from_tokens(tokens)
        pre = get_preorder(root)
        ino = get_inorder(root)
        post = get_postorder(root)
        height = get_tree_height(root)
        serialized = serialize_tree(root)

        nulls_count = tokens.count("#")

        return jsonify({
            "success": True,
            "tree": root.to_dict() if root else None,
            "traversals": {
                "preorder": pre,
                "inorder": ino,
                "postorder": post
            },
            "serialized": serialized,
            "height": height,
            "stats": {
                "nodes": len(pre),
                "height": height,
                "nulls": nulls_count
            }
        })
    except ValueError as e:
        return jsonify({"success": False, "error": str(e)}), 400
    except Exception as e:
        return jsonify({"success": False, "error": f"Internal parsing anomaly: {str(e)}"}), 500

@app.route("/api/tree/serialize", methods=["POST", "OPTIONS"])
def serialize_endpoint():
    if request.method == "OPTIONS":
        return "", 200

    data = request.get_json(silent=True) or {}
    tokens_str = data.get("tokens", "").strip()
    if not tokens_str:
        return jsonify({"success": False, "error": "No tokens supplied."}), 400

    try:
        tokens = tokens_str.split()
        root = build_tree_from_tokens(tokens)
        serialized = serialize_tree(root)
        return jsonify({
            "success": True,
            "serialized": serialized
        })
    except Exception as e:
        return jsonify({"success": False, "error": str(e)}), 400

@app.route("/api/tree/restore", methods=["POST", "OPTIONS"])
def restore_endpoint():
    if request.method == "OPTIONS":
        return "", 200

    data = request.get_json(silent=True) or {}
    archive_text = data.get("archive_text", "").strip()

    if not archive_text:
        return jsonify({"success": False, "error": "Archive manuscript scroll is empty."}), 400

    tokens = archive_text.split()
    try:
        root = build_tree_from_tokens(tokens)
        pre = get_preorder(root)
        ino = get_inorder(root)
        post = get_postorder(root)
        height = get_tree_height(root)

        return jsonify({
            "success": True,
            "tree": root.to_dict() if root else None,
            "traversals": {
                "preorder": pre,
                "inorder": ino,
                "postorder": post
            },
            "height": height,
            "stats": {
                "nodes": len(pre),
                "height": height,
                "nulls": tokens.count("#")
            }
        })
    except ValueError as e:
        return jsonify({"success": False, "error": str(e)}), 400
    except Exception as e:
        return jsonify({"success": False, "error": f"Restoration failure: {str(e)}"}), 500

@app.route("/api/tree/validate", methods=["POST", "OPTIONS"])
def validate_endpoint():
    if request.method == "OPTIONS":
        return "", 200

    data = request.get_json(silent=True) or {}
    archive_text = data.get("archive_text", "").strip()

    if not archive_text:
        return jsonify({
            "valid": False,
            "error": "Empty manuscript scroll: No tokens detected."
        }), 400

    tokens = archive_text.split()
    try:
        build_tree_from_tokens(tokens)
        return jsonify({
            "valid": True,
            "message": "Archival stream is structurally intact and satisfies all binary tree invariants."
        })
    except ValueError as e:
        return jsonify({
            "valid": False,
            "error": str(e)
        }), 400
    except Exception as e:
        return jsonify({
            "valid": False,
            "error": f"Structural defect: {str(e)}"
        }), 400

@app.route("/api/tree/compare", methods=["POST", "OPTIONS"])
def compare_endpoint():
    if request.method == "OPTIONS":
        return "", 200

    data = request.get_json(silent=True) or {}
    orig_str = data.get("original", "").strip()
    rest_str = data.get("restored", "").strip()

    try:
        orig_tokens = orig_str.split()
        rest_tokens = rest_str.split()

        orig_root = build_tree_from_tokens(orig_tokens)
        rest_root = build_tree_from_tokens(rest_tokens)

        orig_pre = get_preorder(orig_root)
        rest_pre = get_preorder(rest_root)

        orig_ino = get_inorder(orig_root)
        rest_ino = get_inorder(rest_root)

        match = (orig_pre == rest_pre) and (orig_ino == rest_ino)

        return jsonify({
            "success": True,
            "match": match,
            "original_preorder": orig_pre,
            "restored_preorder": rest_pre,
            "original_inorder": orig_ino,
            "restored_inorder": rest_ino
        })
    except Exception as e:
        return jsonify({"success": False, "error": str(e)}), 400

if __name__ == "__main__":
    port = int(os.environ.get("PORT", 5000))
    print(f"Museum Archival Conservancy Flask Server running on http://127.0.0.1:{port}")
    app.run(host="0.0.0.0", port=port, debug=True)
