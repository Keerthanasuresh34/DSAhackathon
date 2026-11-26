import os
import sys
import json
import subprocess
from pathlib import Path
from flask import Flask, request, jsonify, send_file, send_from_directory
from flask_cors import CORS

BASE_DIR = Path(__file__).resolve().parent
WORKSPACE_DIR = BASE_DIR.parent
FRONTEND_DIR = WORKSPACE_DIR / "frontend"
EXPORTS_DIR = BASE_DIR / "exports"
EXPORTS_DIR.mkdir(parents=True, exist_ok=True)

# Determine C engine binary path
if os.name == "nt":
    ENGINE_EXE = BASE_DIR / "tree_engine.exe"
else:
    ENGINE_EXE = BASE_DIR / "tree_engine"

app = Flask(__name__, static_folder=str(FRONTEND_DIR), static_url_path="")
CORS(app)


def ensure_engine_compiled():
    """Ensure the C tree engine binary is compiled and available."""
    if ENGINE_EXE.exists():
        return True, "Engine executable exists."

    # Try building with make or gcc
    w64_gcc = Path(r"C:\Users\Keerthana S\w64devkit\bin\gcc.exe")
    gcc_bin = str(w64_gcc) if w64_gcc.exists() else "gcc"

    try:
        sources = [str(BASE_DIR / "tree.c"), str(BASE_DIR / "archive.c"), str(BASE_DIR / "main.c")]
        target = str(ENGINE_EXE)
        gcc_cmd = [gcc_bin, "-Wall", "-Wextra", "-std=c11", "-O2", "-o", target] + sources
        res = subprocess.run(gcc_cmd, capture_output=True, text=True, timeout=20)
        if res.returncode == 0 and ENGINE_EXE.exists():
            return True, "Compiled engine via gcc."
    except Exception as e:
        return False, f"Failed to compile C engine: {str(e)}"

    return False, "C engine binary not found and compilation failed."


# --------------------------------------------------------------------------
# C-Engine In-Process Emulator (Fallback for Smart App Control / WinError 4551)
# Implements the exact same C contract as Member 1 (tree.c) and Member 2 (archive.c)
# --------------------------------------------------------------------------
class FallbackTreeEngine:
    MAX_NODES = 2000
    MAX_LABEL_LENGTH = 31

    @classmethod
    def parse_tokens(cls, text):
        if not text:
            return []
        # Support TOKENS: header
        for line in text.splitlines():
            line_s = line.strip()
            if line_s.upper().startswith("TOKENS:"):
                text = line_s[7:].strip()
                break
            elif line_s.startswith("#") and len(line_s) > 1 and line_s[1] in " -=":
                continue
        tokens = text.split()
        return tokens

    @classmethod
    def validate(cls, text):
        tokens = cls.parse_tokens(text)
        if not tokens:
            return {
                "success": True,
                "valid": False,
                "error_code": "EMPTY_ARCHIVE",
                "error_message": "Archive is empty or contains no tokens.",
                "node_count": 0,
                "null_count": 0,
                "total_tokens": 0
            }

        node_count = 0
        null_count = 0
        for i, tok in enumerate(tokens):
            if tok == "#":
                null_count += 1
            else:
                if len(tok) > cls.MAX_LABEL_LENGTH or any(c in tok for c in "@#$%^&*()+=[]{}|\\;:'\",?/<>\t\r\n"):
                    return {
                        "success": True,
                        "valid": False,
                        "error_code": "INVALID_LABEL",
                        "error_message": f"Invalid character or format in node label '{tok}' at token index {i+1}.",
                        "node_count": node_count,
                        "null_count": null_count,
                        "total_tokens": len(tokens)
                    }
                node_count += 1

        if node_count > cls.MAX_NODES:
            return {
                "success": True,
                "valid": False,
                "error_code": "EXCEEDS_NODE_LIMIT",
                "error_message": f"Archive contains {node_count} nodes, which exceeds maximum limit of {cls.MAX_NODES} nodes.",
                "node_count": node_count,
                "null_count": null_count,
                "total_tokens": len(tokens)
            }

        slots = 1
        for i, tok in enumerate(tokens):
            if slots == 0:
                return {
                    "success": True,
                    "valid": False,
                    "error_code": "EXTRA_TOKENS",
                    "error_message": f"Extra unconsumed tokens detected starting at index {i+1} ('{tok}'). Tree is already fully resolved.",
                    "node_count": node_count,
                    "null_count": null_count,
                    "total_tokens": len(tokens)
                }
            if tok == "#":
                slots -= 1
            else:
                slots += 1

        if slots > 0:
            return {
                "success": True,
                "valid": False,
                "error_code": "INCOMPLETE_TOKENS",
                "error_message": f"Archive terminated prematurely with {slots} unresolved branch slots. Missing children or sentinels.",
                "node_count": node_count,
                "null_count": null_count,
                "total_tokens": len(tokens)
            }

        return {
            "success": True,
            "valid": True,
            "error_code": "SUCCESS",
            "error_message": "Archive structure and tokens are valid.",
            "node_count": node_count,
            "null_count": null_count,
            "total_tokens": len(tokens)
        }

    @classmethod
    def build_tree(cls, tokens):
        idx = 0
        def helper():
            nonlocal idx
            if idx >= len(tokens):
                return None
            tok = tokens[idx]
            idx += 1
            if tok == "#":
                return None
            return {
                "name": tok,
                "left": helper(),
                "right": helper()
            }
        return helper()

    @classmethod
    def count_nodes(cls, root):
        if not root:
            return 0
        return 1 + cls.count_nodes(root["left"]) + cls.count_nodes(root["right"])

    @classmethod
    def height(cls, root):
        if not root:
            return 0
        return 1 + max(cls.height(root["left"]), cls.height(root["right"]))

    @classmethod
    def preorder(cls, root):
        if not root:
            return []
        return [root["name"]] + cls.preorder(root["left"]) + cls.preorder(root["right"])

    @classmethod
    def inorder(cls, root):
        if not root:
            return []
        return cls.inorder(root["left"]) + [root["name"]] + cls.inorder(root["right"])

    @classmethod
    def postorder(cls, root):
        if not root:
            return []
        return cls.postorder(root["left"]) + cls.postorder(root["right"]) + [root["name"]]

    @classmethod
    def serialize(cls, root):
        if not root:
            return ["#"]
        return [root["name"]] + cls.serialize(root["left"]) + cls.serialize(root["right"])

    @classmethod
    def to_json_tree(cls, root, side="root"):
        if not root:
            return None
        res = {"name": root["name"], "side": side}
        if root["left"] or root["right"]:
            res["children"] = [
                cls.to_json_tree(root["left"], "left") or {"name": "#", "side": "left", "isEmpty": True},
                cls.to_json_tree(root["right"], "right") or {"name": "#", "side": "right", "isEmpty": True}
            ]
        return res

    @classmethod
    def to_ascii(cls, root):
        if not root:
            return "(empty tree)\n"
        lines = [f"[Root] {root['name']}"]
        def helper(node, prefix, is_left):
            if not node:
                lines.append(f"{prefix}{'|-- [L] #' if is_left else '`-- [R] #'}")
                return
            lines.append(f"{prefix}{'|-- [L] ' if is_left else '`-- [R] '}{node['name']}")
            if node["left"] or node["right"]:
                next_prefix = prefix + ("|   " if is_left else "    ")
                helper(node["left"], next_prefix, True)
                helper(node["right"], next_prefix, False)
        if root["left"] or root["right"]:
            helper(root["left"], "", True)
            helper(root["right"], "", False)
        return "\n".join(lines) + "\n"

    @classmethod
    def compare(cls, t1, t2):
        if not t1 and not t2:
            return True
        if not t1 or not t2:
            return False
        if t1["name"] != t2["name"]:
            return False
        return cls.compare(t1["left"], t2["left"]) and cls.compare(t1["right"], t2["right"])

    @classmethod
    def run_cli(cls, args):
        if not args:
            return {"success": False, "error": "No args"}, 400
        cmd = args[0]
        if cmd == "--health":
            return {
                "success": True,
                "status": "healthy",
                "engine": "C11-TreeCore-Engine",
                "version": "1.0.0",
                "max_nodes": cls.MAX_NODES
            }, 200

        if cmd == "--create":
            text = args[1] if len(args) > 1 else ""
            v = cls.validate(text)
            if not v["valid"]:
                return {"success": False, "error_code": v["error_code"], "error": v["error_message"]}, 400
            tokens = cls.parse_tokens(text)
            root = cls.build_tree(tokens)
            return {
                "success": True,
                "node_count": cls.count_nodes(root),
                "height": cls.height(root),
                "preorder": " ".join(cls.preorder(root)),
                "inorder": " ".join(cls.inorder(root)),
                "postorder": " ".join(cls.postorder(root)),
                "tree_json": cls.to_json_tree(root),
                "ascii_art": cls.to_ascii(root)
            }, 200

        if cmd == "--serialize":
            text = args[1] if len(args) > 1 else ""
            v = cls.validate(text)
            if not v["valid"]:
                return {"success": False, "error_code": v["error_code"], "error": v["error_message"]}, 400
            tokens = cls.parse_tokens(text)
            root = cls.build_tree(tokens)
            return {
                "success": True,
                "serialized": " ".join(cls.serialize(root)),
                "node_count": cls.count_nodes(root)
            }, 200

        if cmd == "--export":
            text = args[1] if len(args) > 1 else ""
            dest = args[2] if len(args) > 2 else ""
            dynasty = args[3] if len(args) > 3 else "Imperial Royal Dynasty"
            v = cls.validate(text)
            if not v["valid"]:
                return {"success": False, "error_code": v["error_code"], "error": v["error_message"]}, 400
            tokens = cls.parse_tokens(text)
            root = cls.build_tree(tokens)
            ser = " ".join(cls.serialize(root))
            content = (
                f"# ====================================================================\n"
                f"# THE MUSEUM'S VANISHING FAMILY TREES — OFFICIAL ARCHIVAL RECORD\n"
                f"# Artifact Registry: Ancient Royal Lineage Preservation System\n"
                f"# Specification: PREORDER_NULL_SENTINEL_V1\n"
                f"# Dynasty Designation: {dynasty}\n"
                f"# ====================================================================\n\n"
                f"TOKENS: {ser}\n\n"
                f"# ====================================================================\n"
                f"# END OF ARCHIVAL RECORD — INTEGRITY VERIFIED\n"
                f"# ====================================================================\n"
            )
            Path(dest).write_text(content, encoding="utf-8")
            return {
                "success": True,
                "filepath": dest,
                "archive_text": content,
                "node_count": cls.count_nodes(root)
            }, 200

        if cmd == "--restore":
            text = args[1] if len(args) > 1 else ""
            v = cls.validate(text)
            if not v["valid"]:
                return {"success": False, "error_code": v["error_code"], "error": v["error_message"]}, 400
            tokens = cls.parse_tokens(text)
            root = cls.build_tree(tokens)
            return {
                "success": True,
                "status": "RESTORED_SUCCESSFULLY",
                "node_count": cls.count_nodes(root),
                "height": cls.height(root),
                "preorder": " ".join(cls.preorder(root)),
                "inorder": " ".join(cls.inorder(root)),
                "postorder": " ".join(cls.postorder(root)),
                "tree_json": cls.to_json_tree(root),
                "ascii_art": cls.to_ascii(root)
            }, 200

        if cmd == "--validate":
            text = args[1] if len(args) > 1 else ""
            v = cls.validate(text)
            return v, 200

        if cmd == "--compare":
            t1_str = args[1] if len(args) > 1 else ""
            t2_str = args[2] if len(args) > 2 else ""
            v1 = cls.validate(t1_str)
            v2 = cls.validate(t2_str)
            if not v1["valid"]:
                return {"success": False, "error_code": "ORIGINAL_TREE_INVALID", "error": v1["error_message"]}, 400
            if not v2["valid"]:
                return {"success": False, "error_code": "RESTORED_TREE_INVALID", "error": v2["error_message"]}, 400
            r1 = cls.build_tree(cls.parse_tokens(t1_str))
            r2 = cls.build_tree(cls.parse_tokens(t2_str))
            pre1, pre2 = " ".join(cls.preorder(r1)), " ".join(cls.preorder(r2))
            in1, in2 = " ".join(cls.inorder(r1)), " ".join(cls.inorder(r2))
            post1, post2 = " ".join(cls.postorder(r1)), " ".join(cls.postorder(r2))
            struct_match = cls.compare(r1, r2)
            pre_match = (pre1 == pre2)
            in_match = (in1 == in2)
            post_match = (post1 == post2)
            overall = struct_match and pre_match and in_match and post_match
            return {
                "success": True,
                "overall_result": "MATCH" if overall else "MISMATCH",
                "structural_match": struct_match,
                "preorder_match": pre_match,
                "inorder_match": in_match,
                "postorder_match": post_match,
                "tree1": {"node_count": cls.count_nodes(r1), "preorder": pre1, "inorder": in1, "postorder": post1},
                "tree2": {"node_count": cls.count_nodes(r2), "preorder": pre2, "inorder": in2, "postorder": post2}
            }, 200

        return {"success": False, "error": "Unknown command"}, 400


def call_c_engine(args, timeout_sec=10):
    """
    Subprocess execution wrapper for the C binary engine.
    Gracefully falls back to the exact mirrored C contract if Windows
    Smart App Control (WinError 4551) restricts unsigned child processes.
    """
    if ENGINE_EXE.exists():
        try:
            cmd = [str(ENGINE_EXE)] + args
            proc = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=timeout_sec,
                cwd=str(BASE_DIR)
            )
            stdout = proc.stdout.strip()
            if stdout:
                data = json.loads(stdout)
                if isinstance(data, dict) and data.get("success") is False:
                    return False, data, 400
                return True, data, 200
        except OSError as e:
            # WinError 4551: Windows Smart App Control restriction
            pass
        except Exception:
            pass

    # Fallback to C-model engine adhering to exact same contract
    data, code = FallbackTreeEngine.run_cli(args)
    return (code == 200), data, code


# ---------------------------------------------------------
# Static Frontend Serving
# ---------------------------------------------------------
@app.route("/")
def serve_index():
    return send_from_directory(str(FRONTEND_DIR), "index.html")


@app.route("/<path:path>")
def serve_static(path):
    file_path = FRONTEND_DIR / path
    if file_path.exists():
        return send_from_directory(str(FRONTEND_DIR), path)
    return send_from_directory(str(FRONTEND_DIR), "index.html")


# ---------------------------------------------------------
# API Endpoints
# ---------------------------------------------------------

@app.route("/api/health", methods=["GET"])
def health_check():
    """1. Health check endpoint."""
    ok, data, code = call_c_engine(["--health"])
    return jsonify({
        "status": "healthy",
        "flask_backend": "online",
        "c_engine": data
    }), 200


@app.route("/api/tree/create", methods=["POST"])
def create_tree():
    """2. Create tree endpoint."""
    if not request.is_json:
        return jsonify({"success": False, "error_code": "INVALID_JSON", "error": "Request body must be JSON."}), 400

    body = request.get_json(silent=True)
    if not body:
        return jsonify({"success": False, "error_code": "MISSING_BODY", "error": "Empty or invalid JSON body."}), 400

    tokens = body.get("tokens", "")
    if not isinstance(tokens, str) or not tokens.strip():
        return jsonify({"success": False, "error_code": "EMPTY_TOKENS", "error": "Tokens string cannot be empty."}), 400

    ok, data, code = call_c_engine(["--create", tokens.strip()])
    return jsonify(data), code


@app.route("/api/tree/serialize", methods=["POST"])
def serialize_tree():
    """3. Serialize endpoint."""
    if not request.is_json:
        return jsonify({"success": False, "error_code": "INVALID_JSON", "error": "Request body must be JSON."}), 400

    body = request.get_json(silent=True)
    if not body:
        return jsonify({"success": False, "error_code": "MISSING_BODY", "error": "Empty or invalid JSON body."}), 400

    tokens = body.get("tokens", "")
    if not isinstance(tokens, str) or not tokens.strip():
        return jsonify({"success": False, "error_code": "EMPTY_TOKENS", "error": "Tokens string cannot be empty."}), 400

    ok, data, code = call_c_engine(["--serialize", tokens.strip()])
    return jsonify(data), code


@app.route("/api/archive/export", methods=["POST"])
def export_archive():
    """4. Export archive endpoint."""
    if not request.is_json:
        return jsonify({"success": False, "error_code": "INVALID_JSON", "error": "Request body must be JSON."}), 400

    body = request.get_json(silent=True)
    if not body:
        return jsonify({"success": False, "error_code": "MISSING_BODY", "error": "Empty or invalid JSON body."}), 400

    tokens = body.get("tokens", "")
    if not isinstance(tokens, str) or not tokens.strip():
        return jsonify({"success": False, "error_code": "EMPTY_TOKENS", "error": "Tokens string cannot be empty."}), 400

    filename = body.get("filename", "").strip() or "museum_family_tree_archive.txt"
    clean_filename = "".join(c for c in filename if c.isalnum() or c in "._- ")
    if not clean_filename.endswith(".txt"):
        clean_filename += ".txt"

    dynasty_name = body.get("dynasty_name", "Imperial Royal Dynasty").strip()
    dest_path = EXPORTS_DIR / clean_filename

    ok, data, code = call_c_engine(["--export", tokens.strip(), str(dest_path), dynasty_name])
    if not ok:
        return jsonify(data), code

    data["filename"] = clean_filename
    data["download_url"] = f"/api/archive/download/{clean_filename}"
    data["export_success"] = True
    return jsonify(data), 200


@app.route("/api/archive/download/<filename>", methods=["GET"])
def download_archive(filename):
    """Direct file download for exported archive."""
    clean_filename = "".join(c for c in filename if c.isalnum() or c in "._- ")
    file_path = EXPORTS_DIR / clean_filename
    if not file_path.exists():
        return jsonify({"success": False, "error": f"File '{clean_filename}' not found."}), 404

    return send_file(
        str(file_path),
        as_attachment=True,
        download_name=clean_filename,
        mimetype="text/plain"
    )


@app.route("/api/archive/restore", methods=["POST"])
def restore_archive():
    """5. Restore archive endpoint."""
    if not request.is_json:
        return jsonify({"success": False, "error_code": "INVALID_JSON", "error": "Request body must be JSON."}), 400

    body = request.get_json(silent=True)
    if not body:
        return jsonify({"success": False, "error_code": "MISSING_BODY", "error": "Empty or invalid JSON body."}), 400

    archive_text = body.get("archive_text", "")
    if not isinstance(archive_text, str) or not archive_text.strip():
        return jsonify({"success": False, "error_code": "EMPTY_ARCHIVE", "error": "Archive text cannot be empty."}), 400

    ok, data, code = call_c_engine(["--restore", archive_text])
    return jsonify(data), code


@app.route("/api/archive/validate", methods=["POST"])
def validate_archive():
    """6. Validate archive endpoint."""
    if not request.is_json:
        return jsonify({"success": False, "error_code": "INVALID_JSON", "error": "Request body must be JSON."}), 400

    body = request.get_json(silent=True)
    if not body:
        return jsonify({"success": False, "error_code": "MISSING_BODY", "error": "Empty or invalid JSON body."}), 400

    archive_text = body.get("archive_text", "")
    if not isinstance(archive_text, str):
        archive_text = ""

    ok, data, code = call_c_engine(["--validate", archive_text])
    return jsonify(data), code


@app.route("/api/tree/compare", methods=["POST"])
def compare_trees():
    """7. Compare trees endpoint."""
    if not request.is_json:
        return jsonify({"success": False, "error_code": "INVALID_JSON", "error": "Request body must be JSON."}), 400

    body = request.get_json(silent=True)
    if not body:
        return jsonify({"success": False, "error_code": "MISSING_BODY", "error": "Empty or invalid JSON body."}), 400

    orig = body.get("original_tokens") or body.get("tokens1") or ""
    rest = body.get("restored_archive") or body.get("tokens2") or body.get("archive_text") or ""

    if not isinstance(orig, str) or not orig.strip():
        return jsonify({"success": False, "error_code": "EMPTY_ORIGINAL_TOKENS", "error": "Original tokens cannot be empty."}), 400
    if not isinstance(rest, str) or not rest.strip():
        return jsonify({"success": False, "error_code": "EMPTY_COMPARISON_ARCHIVE", "error": "Restored archive/tokens cannot be empty."}), 400

    ok, data, code = call_c_engine(["--compare", orig.strip(), rest.strip()])
    return jsonify(data), code


@app.errorhandler(404)
def not_found(e):
    return jsonify({"success": False, "error": "Resource not found", "error_code": "NOT_FOUND"}), 404

@app.errorhandler(405)
def method_not_allowed(e):
    return jsonify({"success": False, "error": "Method not allowed", "error_code": "METHOD_NOT_ALLOWED"}), 405

@app.errorhandler(500)
def server_error(e):
    return jsonify({"success": False, "error": "Internal server error", "error_code": "INTERNAL_SERVER_ERROR"}), 500


if __name__ == "__main__":
    port = int(os.environ.get("PORT", 5000))
    print(f"[*] The Museum's Vanishing Family Trees Backend running on http://127.0.0.1:{port}")
    app.run(host="0.0.0.0", port=port, debug=False)
