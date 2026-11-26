import pytest

def test_damaged_01_incomplete_tokens(client):
    """1. Damaged Archive: Incomplete tokens (ends prematurely)."""
    # Missing last closing sentinel
    archive = "A B # D # # C #"
    res = client.post("/api/archive/validate", json={"archive_text": archive})
    assert res.status_code == 200
    data = res.get_json()
    assert data["valid"] is False
    assert data["error_code"] == "INCOMPLETE_TOKENS"

    # Restoration should be rejected
    res_restore = client.post("/api/archive/restore", json={"archive_text": archive})
    assert res_restore.status_code == 400
    data_res = res_restore.get_json()
    assert data_res["success"] is False

def test_damaged_02_extra_tokens(client):
    """2. Damaged Archive: Extra tokens after complete tree resolution."""
    archive = "A B # D # # C # # EXTRA_TOKEN"
    res = client.post("/api/archive/validate", json={"archive_text": archive})
    assert res.status_code == 200
    data = res.get_json()
    assert data["valid"] is False
    assert data["error_code"] == "EXTRA_TOKENS"

    res_restore = client.post("/api/archive/restore", json={"archive_text": archive})
    assert res_restore.status_code == 400

def test_damaged_03_invalid_labels(client):
    """3. Damaged Archive: Invalid glyphs or characters in node label."""
    archive = "A B@#$ # D # # C # #"
    res = client.post("/api/archive/validate", json={"archive_text": archive})
    assert res.status_code == 200
    data = res.get_json()
    assert data["valid"] is False
    assert data["error_code"] == "INVALID_LABEL"

    res_restore = client.post("/api/archive/restore", json={"archive_text": archive})
    assert res_restore.status_code == 400

def test_damaged_04_empty_archive(client):
    """4. Damaged Archive: Empty archive string."""
    res = client.post("/api/archive/validate", json={"archive_text": "   "})
    assert res.status_code == 200
    data = res.get_json()
    assert data["valid"] is False
    assert data["error_code"] == "EMPTY_ARCHIVE"

    res_restore = client.post("/api/archive/restore", json={"archive_text": ""})
    assert res_restore.status_code == 400

def test_damaged_05_invalid_structure(client):
    """5. Damaged Archive: Invalid single token without sentinels."""
    archive = "A"
    res = client.post("/api/archive/validate", json={"archive_text": archive})
    assert res.status_code == 200
    data = res.get_json()
    assert data["valid"] is False
    assert data["error_code"] == "INCOMPLETE_TOKENS"

def test_damaged_06_more_than_2000_nodes(client):
    """6. Damaged Archive: More than 2,000 nodes exceeds maximum capacity."""
    # Build 2005 nodes
    tokens = []
    for i in range(2005):
        tokens.append(f"N{i}")
        tokens.append("#")
    tokens.append("#")
    archive = " ".join(tokens)

    res = client.post("/api/archive/validate", json={"archive_text": archive})
    assert res.status_code == 200
    data = res.get_json()
    assert data["valid"] is False
    assert data["error_code"] == "EXCEEDS_NODE_LIMIT"

    res_restore = client.post("/api/archive/restore", json={"archive_text": archive})
    assert res_restore.status_code == 400

def test_damaged_07_file_read_failure(client):
    """7. Damaged Archive: Non-existent file read failure returns 404."""
    res = client.get("/api/archive/download/non_existent_parchment_999.txt")
    assert res.status_code == 404
    data = res.get_json()
    assert data["success"] is False
