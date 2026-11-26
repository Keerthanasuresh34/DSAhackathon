import pytest
import os
from pathlib import Path

def test_full_roundtrip_recovery_workflow(client):
    """
    Exact 10-step round-trip test workflow on an unseen royal dynasty tree
    with asymmetric left-only and right-only branches.
    """
    # 1. Create an unseen tree with both left-only and right-only subtrees
    # Lineage structure:
    #             DynastyRoot
    #             /         \
    #       LeftPatriarch    RightMatriarch
    #       /                      \
    #   LeftOnlySub             RightOnlySub
    unseen_tokens = "DynastyRoot LeftPatriarch LeftOnlySub # # # RightMatriarch # RightOnlySub # #"

    res_create = client.post("/api/tree/create", json={"tokens": unseen_tokens})
    assert res_create.status_code == 200
    orig_tree = res_create.get_json()
    assert orig_tree["success"] is True
    orig_node_count = orig_tree["node_count"]
    orig_preorder = orig_tree["preorder"]
    orig_inorder = orig_tree["inorder"]
    orig_postorder = orig_tree["postorder"]

    assert orig_node_count == 5
    assert orig_preorder == "DynastyRoot LeftPatriarch LeftOnlySub RightMatriarch RightOnlySub"
    assert orig_inorder == "LeftOnlySub LeftPatriarch DynastyRoot RightMatriarch RightOnlySub"
    assert orig_postorder == "LeftOnlySub LeftPatriarch RightOnlySub RightMatriarch DynastyRoot"

    # 2. Generate its serialization
    res_serialize = client.post("/api/tree/serialize", json={"tokens": unseen_tokens})
    assert res_serialize.status_code == 200
    serialized_data = res_serialize.get_json()
    assert serialized_data["success"] is True
    serialized_tokens = serialized_data["serialized"]
    assert serialized_tokens == unseen_tokens

    # 3. Export it to text / file
    export_filename = "unseen_dynasty_roundtrip.txt"
    res_export = client.post("/api/archive/export", json={
        "tokens": serialized_tokens,
        "filename": export_filename,
        "dynasty_name": "Unseen Asymmetric Dynastic Lineage"
    })
    assert res_export.status_code == 200
    export_data = res_export.get_json()
    assert export_data["success"] is True
    exported_archive_text = export_data["archive_text"]

    # 4. Clear the original in-memory tree (simulate parchment vanishing / loss)
    del orig_tree

    # 5. Restore from the exported text alone
    res_restore = client.post("/api/archive/restore", json={"archive_text": exported_archive_text})
    assert res_restore.status_code == 200
    restored_tree = res_restore.get_json()
    assert restored_tree["success"] is True

    # 6. Verify the structure is identical (via compare endpoint and node counts)
    res_compare = client.post("/api/tree/compare", json={
        "original_tokens": unseen_tokens,
        "restored_archive": exported_archive_text
    })
    assert res_compare.status_code == 200
    comp_data = res_compare.get_json()

    assert comp_data["overall_result"] == "MATCH"
    assert comp_data["structural_match"] is True

    # 7. Verify preorder is identical
    assert comp_data["preorder_match"] is True
    assert restored_tree["preorder"] == orig_preorder

    # 8. Verify inorder is identical
    assert comp_data["inorder_match"] is True
    assert restored_tree["inorder"] == orig_inorder

    # 9. Verify postorder is identical
    assert comp_data["postorder_match"] is True
    assert restored_tree["postorder"] == orig_postorder

    # 10. Report successful recovery
    print("\n[SUCCESS] Round-trip archival recovery workflow verified with 100% loss-free isomorphism.")
