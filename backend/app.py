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
    try:
        # Check if make is available
        res = subprocess.run(
            ["make", "-C", str(BASE_DIR)],
            capture_output=True,
            text=True,
            timeout=20
        )
        if res.returncode == 0 and ENGINE_EXE.exists():
            return True, "Compiled engine via Makefile."
    except Exception:
        pass

    # Fallback direct gcc compilation
    try:
        sources = [str(BASE_DIR / "tree.c"), str(BASE_DIR / "archive.c"), str(BASE_DIR / "main.c")]
        target = str(ENGINE_EXE)
        gcc_cmd = ["gcc", "-Wall", "-Wextra", "-std=c11", "-O2", "-o", target] + sources
        res = subprocess.run(gcc_cmd, capture_output=True, text=True, timeout=20)
        if res.returncode == 0 and ENGINE_EXE.exists():
            return True, "Compiled engine via gcc fallback."
    except Exception as e:
        return False, f"Failed to compile C engine: {str(e)}"

    return False, "C engine binary not found and compilation failed."


def call_c_engine(args, timeout_sec=10):
    """
    Subprocess execution wrapper for the C binary engine.
    Returns (success_bool, parsed_json_or_error_dict, status_code).
    """
    if not ENGINE_EXE.exists():
        ok, msg = ensure_engine_compiled()
        if not ok:
            return False, {
                "success": False,
                "error_code": "ENGINE_UNAVAILABLE",
                "error": f"C engine binary is unavailable: {msg}"
            }, 503

    cmd = [str(ENGINE_EXE)] + args
    try:
        proc = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=timeout_sec,
            cwd=str(BASE_DIR)
        )
    except subprocess.TimeoutExpired:
        return False, {
            "success": False,
            "error_code": "PROCESS_TIMEOUT",
            "error": "The backend C engine timed out while processing request."
        }, 504
    except Exception as e:
        return False, {
            "success": False,
            "error_code": "PROCESS_FAILURE",
            "error": f"Failed to execute backend process: {str(e)}"
        }, 500

    stdout = proc.stdout.strip()
    stderr = proc.stderr.strip()

    if not stdout:
        return False, {
            "success": False,
            "error_code": "EMPTY_ENGINE_OUTPUT",
            "error": stderr or "C engine returned no output."
        }, 500

    try:
        data = json.loads(stdout)
        if isinstance(data, dict) and data.get("success") is False:
            return False, data, 400
        return True, data, 200
    except json.JSONDecodeError:
        return False, {
            "success": False,
            "error_code": "INVALID_JSON_FROM_ENGINE",
            "error": "Failed to parse JSON response from C engine.",
            "raw_output": stdout
        }, 500


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
    if not ok:
        return jsonify({
            "status": "degraded",
            "flask_backend": "online",
            "c_engine": "error",
            "details": data
        }), code

    return jsonify({
        "status": "healthy",
        "flask_backend": "online",
        "c_engine": data
    }), 200


@app.route("/api/tree/create", methods=["POST"])
def create_tree():
    """
    2. Create tree endpoint.
    Input: {"tokens": "A B # D # # C # #"}
    """
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
    """
    3. Serialize endpoint.
    Input: {"tokens": "A B # D # # C # #"}
    """
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
    """
    4. Export archive endpoint.
    Input: {"tokens": "A B # D # # C # #", "filename": "royal_dynasty.txt", "dynasty_name": "..."}
    """
    if not request.is_json:
        return jsonify({"success": False, "error_code": "INVALID_JSON", "error": "Request body must be JSON."}), 400

    body = request.get_json(silent=True)
    if not body:
        return jsonify({"success": False, "error_code": "MISSING_BODY", "error": "Empty or invalid JSON body."}), 400

    tokens = body.get("tokens", "")
    if not isinstance(tokens, str) or not tokens.strip():
        return jsonify({"success": False, "error_code": "EMPTY_TOKENS", "error": "Tokens string cannot be empty."}), 400

    filename = body.get("filename", "").strip() or "museum_family_tree_archive.txt"
    # Sanitize filename
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
    """
    5. Restore archive endpoint.
    Input: {"archive_text": "..."}
    """
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
    """
    6. Validate archive endpoint.
    Input: {"archive_text": "..."}
    """
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
    """
    7. Compare trees endpoint.
    Input:
    {
      "original_tokens": "A B # D # # C # #",
      "restored_archive": "..."
    }
    """
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


# ---------------------------------------------------------
# Error Handlers
# ---------------------------------------------------------
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
    ensure_engine_compiled()
    port = int(os.environ.get("PORT", 5000))
    print(f"[*] The Museum's Vanishing Family Trees Backend running on http://127.0.0.1:{port}")
    app.run(host="0.0.0.0", port=port, debug=False)
