# The Museum's Vanishing Family Trees — Tree Archiving & Recovery System

[![C11](https://img.shields.io/badge/Language-C11-00599C?logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Python 3.12](https://img.shields.io/badge/Backend-Python%203.12-3776AB?logo=python&logoColor=white)](https://python.org)
[![Flask API](https://img.shields.io/badge/Framework-Flask%203.1-000000?logo=flask&logoColor=white)](https://flask.palletsprojects.com/)
[![Tests Passing](https://img.shields.io/badge/Tests-18%2F18%20Passing-brightgreen?logo=pytest&logoColor=white)](backend/tests/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

---

## 1. Project Title
**"The Museum's Vanishing Family Trees — Tree Archiving and Recovery System"**  
*An End-to-End Binary Tree Archival, Forensic Damage Detection, and Lossless Dynastic Recovery Platform.*

---

## 2. Problem Statement
Ancient royal genealogical scrolls, dynastic family trees, and historical parchment records housed in world museums are physically fragile and subject to decay, water damage, ink fading, and physical tearing—termed the **"Vanishing Family Trees."** 

When historical genealogies vanish or get partially obliterated:
- Hierarchical succession paths and parent-child linkages are lost.
- Partial transcriptions often end up with missing branches, corrupted glyphs, or spurious trailing notes.
- Historians and curators require a mathematically proven, tamper-evident computational system to serialize hierarchical family trees into archival records, detect and pinpoint manuscript damage with forensic precision, and restore identical trees with 100% structural and traversal isomorphism.

---

## 3. Project Objectives
1. **Source of Truth in C**: Maintain memory-efficient, high-performance C11 binary tree operations (`tree.c`, `archive.c`) as the definitive engine for tree construction, serialization, and recovery.
2. **Robust Archival Specification**: Define an unambiguous serialization standard leveraging **Preorder Traversal with Null-Sentinels (`#`)**, satisfying the binary tree topological parity equation $N_{\text{sentinels}} = N_{\text{nodes}} + 1$.
3. **Forensic Damage Detection**: Diagnose corrupted archives (incomplete streams, extra tokens, illegal character glyphs, malformed topology, and excessive node limits).
4. **Seamless Integration**: Connect the C engine with a Python Flask REST API via secure `subprocess` execution without rewriting C logic in Python.
5. **Interactive Museum Exhibition Interface**: Provide an imperial obsidian / ancient parchment UI with live SVG tree rendering, animated traversal stepped playback, and one-click round-trip proof verification.
6. **Exhaustive Automated Verification**: Achieve 100% test pass rates across valid topologies, damaged records, and unseen asymmetric lineages.

---

## 4. Key Features
- **Dynamic Preorder Deserializer**: Reconstructs binary trees in $O(N)$ time from token streams containing null sentinels (`#`).
- **Real-Time Traversal Engine**: Simultaneously extracts Preorder, Inorder, and Postorder traversals, height, and node counts.
- **Museum Archival Records**: Generates formal, timestamped parchment records with metadata headers and raw token payloads.
- **Forensic Validation Sandbox**: Pinpoints corrupted tokens, premature terminations, unconsumed tokens, and non-alphanumeric symbols.
- **Tree Isomorphism Verifier**: Compares original and recovered trees across structural topology and all 3 traversals.
- **Animated Traversal Player**: Visually illuminates monarchs on the SVG canvas step-by-step in Preorder, Inorder, or Postorder sequence.
- **Dual Exhibition Themes**: Switch between **Imperial Obsidian Hall** (dark mode) and **Ancient Alexandria Parchment** (antique sepia).

---

## 5. Technology Stack
- **Core Engine**: C (C11 standard, compiled with `-Wall -Wextra -std=c11 -O2`).
- **Backend API**: Python 3.12, Flask 3.1, Flask-CORS.
- **Build System**: GNU Make / GCC (MinGW-w64 on Windows, native GCC/Clang on Linux/macOS).
- **Frontend**: Semantic HTML5, CSS3 Custom Properties (BEM-inspired), Vanilla JavaScript (ES6+), Dynamic SVG graphics.
- **Testing**: PyTest with automated test client fixtures.

---

## 6. Architecture

```
                  ┌─────────────────────────────────────────┐
                  │    Museum Exhibition UI (HTML5/SVG/JS)   │
                  │  [Interactive Canvas | Sandbox | Forensics]│
                  └───────────────────▲─────────────────────┘
                                      │  JSON REST API
                                      ▼
                  ┌─────────────────────────────────────────┐
                  │       Flask Backend (backend/app.py)     │
                  │  [Input Validation | CORS | Process Hub] │
                  └───────────────────▲─────────────────────┘
                                      │  Subprocess / CLI JSON Protocol
                                      ▼
                  ┌─────────────────────────────────────────┐
                  │     C Integration CLI (backend/main.c)   │
                  └──────────────┬──────────────────┬───────┘
                                 │                  │
                                 ▼                  ▼
                    ┌─────────────────┐   ┌─────────────────┐
                    │ Tree Core (C11) │   │ Archive Engine  │
                    │  tree.h/tree.c  │   │archive.h/archive.c
                    └─────────────────┘   └─────────────────┘
```

---

## 7. Module Responsibilities

| Member | Role | Assigned Files | Responsibilities |
| :--- | :--- | :--- | :--- |
| **Member 1** | Binary Tree Core | `backend/tree.h`, `backend/tree.c` | Recursive node allocation, tree lifecycle (`free_tree`), preorder deserialization, height, traversals (pre/in/post), isomorphism comparison, ASCII/JSON tree generators. |
| **Member 2** | Archive & Recovery | `backend/archive.h`, `backend/archive.c` | Preorder serialization with `#` sentinels, museum document formatting, strict validation rules, slot parity simulation, file I/O safety. |
| **Member 3** | Web UI & Exhibition | `frontend/index.html`, `frontend/style.css`, `frontend/script.js` | Interactive SVG tree renderer, traversal animator, damage sandbox injectors, side-by-side comparison tables, theme switcher. |
| **Member 4** | Backend & Integration | `backend/app.py`, `backend/main.c`, `backend/Makefile`, `backend/tests/`, `requirements.txt`, `README.md`, `.gitignore` | Flask REST API, C CLI wrapper, compilation automation, end-to-end integration, 18-case automated test suite, Windows/POSIX setup. |

---

## 8. Folder Structure

```text
dsa hackathon/
├── backend/
│   ├── app.py                      # Flask REST API server & static file host
│   ├── main.c                      # C integration CLI wrapper (JSON protocol)
│   ├── Makefile                    # Cross-platform compiler automation
│   ├── tree.h                      # Binary tree core header
│   ├── tree.c                      # Binary tree implementation
│   ├── archive.h                   # Archive & validation header
│   ├── archive.c                   # Archive serialization & recovery implementation
│   ├── exports/                    # Directory for exported museum parchment files
│   │   └── .gitkeep
│   └── tests/                      # Automated PyTest integration suite
│       ├── __init__.py
│       ├── conftest.py             # Test fixtures & auto-build triggers
│       ├── test_valid_trees.py     # 10 valid tree topology tests
│       ├── test_damaged_archives.py# 7 damaged archive & edge-case tests
│       └── test_roundtrip.py       # 10-step full round-trip recovery workflow
├── frontend/
│   ├── index.html                  # Exhibition dashboard & controls
│   ├── style.css                   # Obsidian & Parchment styling
│   └── script.js                   # Client controller & SVG visualizer
├── requirements.txt                # Python dependencies
├── .gitignore                      # Git exclusion rules
└── README.md                       # Complete documentation
```

---

## 9. Serialization Format

The system uses **Preorder Traversal with `#` Sentinels**.

### Formal Mathematical Specification:
1. Every non-null monarch node is serialized as its string label.
2. Every empty/missing child pointer is explicitly represented by the sentinel symbol `#`.
3. For any binary tree with $N$ monarch nodes ($N \ge 0$):
   $$\text{Total Tokens} = 2N + 1$$
   $$\text{Null Sentinels (\#)} = N + 1$$
4. A non-empty binary tree stream is valid if and only if, during simulated sequential construction, the required child branch slots remain $> 0$ until the exact final token, at which point the required slots drop to exactly $0$.

---

## 10. Example Archive

### Example Tree Topology:
```text
        A
       / \
      B   C
       \
        D
```

### Serialized Tokens:
```text
A B # D # # C # #
```

### Museum Archival Parchment Record:
```text
# ====================================================================
# THE MUSEUM'S VANISHING FAMILY TREES — OFFICIAL ARCHIVAL RECORD
# Artifact Registry: Ancient Royal Lineage Preservation System
# Specification: PREORDER_NULL_SENTINEL_V1
# Dynasty Designation: Alexandria Ptolemaic Dynasty
# Archival Timestamp: 2026-11-26 11:00:00 UTC
# ====================================================================

TOKENS: A B # D # # C # #

# ====================================================================
# END OF ARCHIVAL RECORD — INTEGRITY VERIFIED
# ====================================================================
```

---

## 11. How to Install Requirements

Ensure **Python 3.10+** is installed on your system.

```bash
# Navigate to the project root
cd "dsa hackathon"

# Install Python requirements
pip install -r requirements.txt
```

---

## 12. How to Compile the C Program

### Windows (MinGW-w64 / GCC):
If using MinGW GCC (or the included w64devkit):
```powershell
cd backend
make
```
*Alternatively, compile directly with GCC:*
```powershell
gcc -Wall -Wextra -std=c11 -O2 -o tree_engine.exe tree.c archive.c main.c
```

### Linux / macOS:
```bash
cd backend
make
```
*Or directly:*
```bash
gcc -Wall -Wextra -std=c11 -O2 -o tree_engine tree.c archive.c main.c
```

---

## 13. How to Run Flask

```bash
# From the project root
python backend/app.py
```

The Flask server will start on `http://127.0.0.1:5000`.

---

## 14. How to Open the Frontend

Simply open your browser and navigate to:
```text
http://127.0.0.1:5000/
```
The Flask backend automatically serves `frontend/index.html` and all static assets!

---

## 15. How to Use the Application

1. **Select or Enter Tokens**: Select an archival preset (e.g., *Museum Sample: Ptolemaic Dynasty*) or enter custom space-separated tokens in the input box.
2. **Construct Tree**: Click **"Construct & Visualize Tree"**. The interactive SVG canvas displays the dynastic tree, and metrics/traversals update immediately.
3. **Animate Traversals**: Click **"▶ Animate"** next to Preorder, Inorder, or Postorder to watch nodes highlight sequentially.
4. **Export Archive**: Click **"Export to Archival Record"** to generate and download a formal museum `.txt` parchment document.
5. **Damage Forensics**: In Panel 4, click any **Damage Sandbox** injector (e.g., *Incomplete Tokens*, *Extra Tokens*, or *Illegal Glyphs*) and click **"Validate Archive Integrity"** to observe forensic rejection.
6. **Full Round-Trip Proof**: Click **"Execute Full Round-Trip Proof"** in Panel 5 to automate creation, serialization, clearing, restoration, and 100% isomorphic verification!

---

## 16. Testing Instructions

Run the automated integration test suite covering all 18 cases:

```bash
# Run all tests with verbose output
python -m pytest backend/tests -v
```

### Test Suite Structure:
- `test_valid_trees.py`: 10 tests for all required valid topologies and export/restore cycles.
- `test_damaged_archives.py`: 7 tests verifying rejection of damaged or corrupt records.
- `test_roundtrip.py`: Complete 10-step round-trip proof on an unseen asymmetric tree.

---

## 17. Expected Traversals

For the standard museum tree `A B # D # # C # #`:

| Traversal | Order of Visiting | Expected Output |
| :--- | :--- | :--- |
| **Preorder** | Root $\rightarrow$ Left $\rightarrow$ Right | `A B D C` |
| **Inorder** | Left $\rightarrow$ Root $\rightarrow$ Right | `B D A C` |
| **Postorder** | Left $\rightarrow$ Right $\rightarrow$ Root | `D B C A` |

For an unseen asymmetric dynasty `DynastyRoot LeftPatriarch LeftOnlySub # # # RightMatriarch # RightOnlySub # #`:

| Traversal | Expected Output |
| :--- | :--- |
| **Preorder** | `DynastyRoot LeftPatriarch LeftOnlySub RightMatriarch RightOnlySub` |
| **Inorder** | `LeftOnlySub LeftPatriarch DynastyRoot RightMatriarch RightOnlySub` |
| **Postorder** | `LeftOnlySub LeftPatriarch RightOnlySub RightMatriarch DynastyRoot` |

---

## 18. Damaged Archive Demonstration

The C validation engine implements a strict state machine checking:
1. **Incomplete Tokens**: Input `A B # D # # C #`  
   *Result*: `{"valid": false, "error_code": "INCOMPLETE_TOKENS", "error_message": "Archive terminated prematurely with 1 unresolved branch slots."}`
2. **Extra Tokens**: Input `A B # D # # C # # EXTRA`  
   *Result*: `{"valid": false, "error_code": "EXTRA_TOKENS", "error_message": "Extra unconsumed tokens detected starting at index 10 ('EXTRA')."}`
3. **Corrupted Glyphs**: Input `A B@#$ # D # # C # #`  
   *Result*: `{"valid": false, "error_code": "INVALID_LABEL", "error_message": "Invalid character or format in node label 'B@#$'."}`
4. **Empty Archive**: Input `""`  
   *Result*: `{"valid": false, "error_code": "EMPTY_ARCHIVE", "error_message": "Archive is empty or contains no tokens."}`
5. **Node Count Ceiling**: Input $> 2,000$ nodes  
   *Result*: `{"valid": false, "error_code": "EXCEEDS_NODE_LIMIT", "error_message": "Archive contains 2005 nodes, which exceeds maximum limit of 2000 nodes."}`

---

## 19. System Limitations
- **Tree Type**: Strictly models **Binary Trees** (each monarch has at most 2 direct successors/branches). General $k$-ary trees must be converted via binary child-sibling representation.
- **Label Limits**: Monarch node labels are bounded to 63 alphanumeric characters (`[a-zA-Z0-9_\-\.]`).
- **Maximum Scale**: Archival files are capped at 2,000 nodes to prevent unbounded memory allocation and stack overflows during deep recursion.
- **Null Symbol**: The character `#` is reserved exclusively as the empty branch sentinel.

---

## 20. Team Member Contributions

- **Member 1 (Binary Tree Core)**:
  - Designed the `TreeNode` structure in `tree.h`.
  - Implemented preorder deserialization, height calculation, and tree deallocation in `tree.c`.
  - Implemented Preorder, Inorder, and Postorder traversal extractors and tree isomorphism matching.
- **Member 2 (Archive & Recovery)**:
  - Designed the `archive.h` specification and museum archival envelope format.
  - Implemented `serialize_tree` and file export/import in `archive.c`.
  - Implemented the branch slot simulator for strict archive validation and corruption diagnostics.
- **Member 3 (Frontend & Visualizer)**:
  - Designed the Alexandria Museum UI theme in `index.html` and `style.css`.
  - Implemented dynamic SVG tree rendering with curved branch connectors and tooltips in `script.js`.
  - Built the traversal animation step player and damage injection sandbox.
- **Member 4 (Backend, Integration & Documentation)**:
  - Designed and implemented the Flask REST API (`backend/app.py`).
  - Created the C CLI integration wrapper (`backend/main.c`) with a structured JSON protocol.
  - Authored the cross-platform `backend/Makefile`.
  - Created the comprehensive 18-case PyTest automated test suite (`backend/tests/`).
  - Authored complete project documentation, requirements, and `.gitignore`.
