# Museum Family Tree Archive Format Specification

**Module:** Member 2 — Serialization, Archive Export, Restoration & Validation  
**Project:** *The Museum's Vanishing Family Trees — Tree Archiving and Recovery System*

---

## 1. Purpose

The archive format stores any binary family tree as plain text in a **lossless** manner. Every present node and every absent (`NULL`) left/right child position is explicitly recorded so the exact tree shape is preserved across export and recovery.

---

## 2. Traversal & Encoding Rules

1. **Preorder Traversal (`Root Left Right`)**:
   - Write the current node's label.
   - Recursively serialize its **left** subtree.
   - Recursively serialize its **right** subtree.
2. **Null Child Marker (`#`)**:
   - Whenever a left or right child pointer is `NULL`, emit the single character `#`.
   - Every actual node has **exactly two** child subtrees accounted for in the token stream.
3. **Empty Tree**:
   - A completely empty tree (`root == NULL`) is represented by the single token `#`.
4. **Token Separation**:
   - Tokens are separated by whitespace (`' '`, `'\t'`, `'\n'`, `'\r'`).
   - During export (`serializeTree`), tokens are joined by single spaces with no trailing spaces.
   - During parsing (`restoreTreeFromText` / `validateArchive`), any consecutive whitespace runs are treated as token separators.

---

## 3. Structural & Label Constraints

| Constraint | Value | Notes |
| --- | --- | --- |
| Maximum actual nodes | `2,000` (`MAX_ARCHIVE_NODES`) | Excludes `#` markers |
| Maximum total tokens (standard stream) | `4,001` (`2 * MAX_ARCHIVE_NODES + 1`) | For $N$ nodes, a complete preorder archive has $N + 1$ `#` markers |
| Maximum label length | `31` characters (`MAX_LABEL_LENGTH - 1`) | Fits safely in `TreeNode.label[32]` |
| Maximum archive size | `100,000` characters (`MAX_ARCHIVE_SIZE`) | Enforced for both text and files |
| Reserved token | `#` | Cannot be used as a node label (nor appear inside a label) |
| Allowed label characters | Printable non-whitespace ASCII (`0x21`–`0x7E`, excluding `#`) | Prevents ambiguity with delimiters and null markers |

---

## 4. Valid Archive Examples

### Example 1: Empty Tree
```text
#
```

### Example 2: Single Node
```text
A # #
```

### Example 3: Left-Only Branch
```text
A B C # # # #
```
- `A` has left child `B` and right child `NULL`
- `B` has left child `C` and right child `NULL`
- `C` has no children

### Example 4: Right-Only Branch
```text
A # B # C # #
```
- `A` has left child `NULL` and right child `B`
- `B` has left child `NULL` and right child `C`
- `C` has no children

### Example 5: Mixed Missing Children
```text
A B # D # # C # #
```
- `A` is root (`left = B`, `right = C`)
- `B` has `left = NULL`, `right = D`
- `D` has `left = NULL`, `right = NULL`
- `C` has `left = NULL`, `right = NULL`

---

## 5. Invalid / Damaged Archive Examples & Rejection Reasons

| Input | Status | Rejection Reason |
| --- | --- | --- |
| `A B #` | Rejected | Incomplete archive: `B` is missing its right child, and `A` is missing its right child |
| `A # # C` | Rejected | Extra tokens after a complete tree (`C` appears after root `A` is already complete) |
| `A B # X # #` | Rejected | Incomplete archive: `A` is missing its right child |
| `` (empty / whitespace) | Rejected | Missing archive data |
| `# #` | Rejected | Extra token `#` after empty tree `#` |
| `A#B # #` | Rejected | Invalid label containing reserved `#` character |
| 32+ char label | Rejected | Label exceeds 31-character maximum |
| 2,001 nodes | Rejected | Exceeds 2,000-node limit |

On any restoration error, all partially constructed `TreeNode` allocations are freed and `NULL` is returned.
