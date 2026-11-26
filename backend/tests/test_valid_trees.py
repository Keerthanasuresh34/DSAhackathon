import pytest
import json

def test_01_empty_tree(client):
    """1. Valid Test: Empty tree (#)."""
    res = client.post("/api/tree/create", json={"tokens": "#"})
    assert res.status_code == 200
    data = res.get_json()
    assert data["success"] is True
    assert data["node_count"] == 0
    assert data["height"] == 0
    assert data["preorder"] == ""
    assert data["inorder"] == ""
    assert data["postorder"] == ""

def test_02_single_node(client):
    """2. Valid Test: Single node (A # #)."""
    res = client.post("/api/tree/create", json={"tokens": "A # #"})
    assert res.status_code == 200
    data = res.get_json()
    assert data["success"] is True
    assert data["node_count"] == 1
    assert data["height"] == 1
    assert data["preorder"] == "A"
    assert data["inorder"] == "A"
    assert data["postorder"] == "A"

def test_03_balanced_tree(client):
    """3. Valid Test: Balanced 7-node tree."""
    tokens = "A B D # # E # # C F # # G # #"
    res = client.post("/api/tree/create", json={"tokens": tokens})
    assert res.status_code == 200
    data = res.get_json()
    assert data["success"] is True
    assert data["node_count"] == 7
    assert data["height"] == 3
    assert data["preorder"] == "A B D E C F G"
    assert data["inorder"] == "D B E A F C G"
    assert data["postorder"] == "D E B F G C A"

def test_04_left_only_child(client):
    """4. Valid Test: Strictly left-only chain (A B C # # # #)."""
    tokens = "A B C # # # #"
    res = client.post("/api/tree/create", json={"tokens": tokens})
    assert res.status_code == 200
    data = res.get_json()
    assert data["success"] is True
    assert data["node_count"] == 3
    assert data["height"] == 3
    assert data["preorder"] == "A B C"
    assert data["inorder"] == "C B A"
    assert data["postorder"] == "C B A"

def test_05_right_only_child(client):
    """5. Valid Test: Strictly right-only chain (A # B # C # #)."""
    tokens = "A # B # C # #"
    res = client.post("/api/tree/create", json={"tokens": tokens})
    assert res.status_code == 200
    data = res.get_json()
    assert data["success"] is True
    assert data["node_count"] == 3
    assert data["height"] == 3
    assert data["preorder"] == "A B C"
    assert data["inorder"] == "A B C"
    assert data["postorder"] == "C B A"

def test_06_missing_left_present_right(client):
    """6. Valid Test: Root missing left child and has present right child (A # B # #)."""
    tokens = "A # B # #"
    res = client.post("/api/tree/create", json={"tokens": tokens})
    assert res.status_code == 200
    data = res.get_json()
    assert data["success"] is True
    assert data["node_count"] == 2
    assert data["height"] == 2
    assert data["preorder"] == "A B"
    assert data["inorder"] == "A B"
    assert data["postorder"] == "B A"

def test_07_missing_right_present_left(client):
    """7. Valid Test: Root missing right child and has present left child (A B # # #)."""
    tokens = "A B # # #"
    res = client.post("/api/tree/create", json={"tokens": tokens})
    assert res.status_code == 200
    data = res.get_json()
    assert data["success"] is True
    assert data["node_count"] == 2
    assert data["height"] == 2
    assert data["preorder"] == "A B"
    assert data["inorder"] == "B A"
    assert data["postorder"] == "B A"

def test_08_sample_museum_tree(client):
    """8. Valid Test: Sample museum tree (A B # D # # C # #)."""
    tokens = "A B # D # # C # #"
    res = client.post("/api/tree/create", json={"tokens": tokens})
    assert res.status_code == 200
    data = res.get_json()
    assert data["success"] is True
    assert data["node_count"] == 4
    assert data["height"] == 3
    assert data["preorder"] == "A B D C"
    assert data["inorder"] == "B D A C"
    assert data["postorder"] == "D B C A"

def test_09_archive_export(client):
    """9. Valid Test: Archive export."""
    tokens = "A B # D # # C # #"
    res = client.post("/api/archive/export", json={
        "tokens": tokens,
        "filename": "test_museum_export.txt",
        "dynasty_name": "Test Ptolemaic Dynasty"
    })
    assert res.status_code == 200
    data = res.get_json()
    assert data["success"] is True
    assert data["export_success"] is True
    assert "TOKENS: A B # D # # C # #" in data["archive_text"]
    assert "download_url" in data

    # Verify download endpoint
    dl_res = client.get(data["download_url"])
    assert dl_res.status_code == 200
    assert b"TOKENS: A B # D # # C # #" in dl_res.data

def test_10_archive_restore(client):
    """10. Valid Test: Archive restore."""
    archive_text = (
        "# --- MUSEUM ARCHIVE ---\n"
        "# DYNASTY: Imperial Dynasty\n"
        "TOKENS: A B # D # # C # #\n"
        "# --- END ---"
    )
    res = client.post("/api/archive/restore", json={"archive_text": archive_text})
    assert res.status_code == 200
    data = res.get_json()
    assert data["success"] is True
    assert data["node_count"] == 4
    assert data["preorder"] == "A B D C"
    assert data["inorder"] == "B D A C"
    assert data["postorder"] == "D B C A"
