# The Museum's Vanishing Family Trees
## Binary Tree Archiving and Recovery System
**College Data Structures Project — Team Member 3 (Frontend UI & Interactive Visualization)**

---

### Project Overview
"The Museum's Vanishing Family Trees" is an archival digital conservation system built to recover, archive, and mathematically verify binary family trees and historical dynasties transcribed in fragile, vanishing manuscripts.

Using **preorder serialization with null-terminating leaf markers (`#`)**, the system allows complete, unambiguous serialization of binary trees into plain text scrolls, and faithful reconstruction with round-trip verification.

---

### Team Roles & Modular Architecture
- **Member 1 & 2:** C Core Algorithms (`tree_node.c`, `serialize.c`, `restore.c`, traversal walks).
- **Member 3 (Assigned):** Complete Museum UI, Interactive SVG Visualization Engine, Dynamic Playback, and Audit Dashboard.
- **Member 4:** Flask REST API Middleware bridging C engine & HTTP client.

---

### Frontend Deliverables (Member 3)
Located inside the `frontend/` directory:
- [`frontend/index.html`](file:///c:/Users/deepi/OneDrive/Desktop/DSA/frontend/index.html) — Museum archival dashboard interface.
- [`frontend/style.css`](file:///c:/Users/deepi/OneDrive/Desktop/DSA/frontend/style.css) — Modern digital museum theme (warm ivory, deep charcoal navy, gold accents, sage green, rounded archival cards, responsive layout).
- [`frontend/script.js`](file:///c:/Users/deepi/OneDrive/Desktop/DSA/frontend/script.js) — Vanilla JavaScript engine driving Fetch API, dynamic SVG hierarchy rendering with pan/zoom, traversal walk animator, dropzone file processing, and Web Audio API synthesizer.

---

### System Features
1. **Accession Desk (Tree Creation):**
   - Supports space-delimited preorder streams (e.g., `A B # D # # C # #`).
   - Live token syntax analysis ($N$ internal nodes require $N+1$ null leaves `#`).
   - Presets for textbook, balanced, skewed, and royal dynasties.
2. **Specimen Chamber (Interactive SVG Tree Visualization):**
   - Renders parent-child links as smooth bezier curves.
   - Distinct styles for sovereign root, royal internal nodes, leaf nodes, and ghost vanished branches (`#`).
   - Pan, zoom, center, toggle ghost markers, and export to `.svg`.
   - Node inspection tooltip displaying depth, subtree count, and child relationships.
3. **Traversal Chronicles:**
   - Preorder ($NLR$), Inorder ($LNR$), and Postorder ($LRN$).
   - Interactive step-by-step traversal playback highlighting corresponding nodes in real time on the SVG canvas.
4. **Preservation Vault (Serialization):**
   - Encapsulates active trees into lossless preorder text streams.
   - Computes SHA-256 integrity checksum and byte size.
   - Downloads authenticated `.txt` archival manuscripts.
5. **Restoration Workshop (Recovery):**
   - Drag-and-drop `.txt` file uploader and manual token transcript paste.
   - Restores tree topology and displays reconstructed preview.
6. **Forensic Audit & Comparison:**
   - Compares Preorder, Inorder, Node Count, Height, and Null Count.
   - Displays curator wax seal verdict: `SUCCESS — STRUCTURE PRESERVED` or `FAILURE — TREE STRUCTURE DIFFERS`.
7. **Damaged Archive Stress Lab:**
   - Tests truncated or corrupted inputs (e.g., `A B #`, `A % ^ # @`).
   - Captures backend rejection diagnostics and highlights invariant violations.

---

### REST API Contract (with Member 4)
| Endpoint | Method | Payload | Response |
| :--- | :--- | :--- | :--- |
| `/api/health` | `GET` | None | `{ "status": "ok", "backend": "C-BinaryTree-Engine" }` |
| `/api/tree/create` | `POST` | `{ "tokens": "A B # D # # C # #" }` | `{ "success": true, "tree": {...}, "traversals": {...}, "stats": {...} }` |
| `/api/tree/serialize`| `POST` | `{ "tokens": "..." }` | `{ "success": true, "serialized": "..." }` |
| `/api/tree/restore` | `POST` | `{ "archive_text": "..." }` | `{ "success": true, "tree": {...}, "traversals": {...} }` |
| `/api/tree/validate`| `POST` | `{ "archive_text": "..." }` | `{ "valid": true/false, "error": "..." }` |
| `/api/tree/compare` | `POST` | `{ "original": "...", "restored": "..." }` | `{ "success": true, "match": true/false }` |

---

### Running the Project
#### 1. Start the Flask Server
```bash
python backend/app.py
```
Server runs on `http://127.0.0.1:5000`.

#### 2. Open in Browser
Visit `http://127.0.0.1:5000` in any web browser to view the live dashboard!
Alternatively, `frontend/index.html` can be opened directly or through any static web server.
