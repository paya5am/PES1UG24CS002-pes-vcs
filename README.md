# Lab: Building PES-VCS — A Version Control System from Scratch

**Objective:** Build a local version control system that tracks file changes, stores snapshots efficiently, and supports commit history. Every component maps directly to operating system and filesystem concepts.

**Platform:** Ubuntu 22.04

---

## Getting Started

### Prerequisites

```bash
sudo apt update && sudo apt install -y gcc build-essential libssl-dev
```

### Using This Repository

This is a **template repository**. Do **not** fork it.

1. Click **"Use this template"** → **"Create a new repository"** on GitHub
2. Name your repository (e.g., `pes-vcs`) and set it to **private**
3. Clone your new repository and start working

The repository contains skeleton source files with `// TODO` markers where you need to write code. Functions marked `// PROVIDED` are complete — do not modify them.

### Building

```bash
make          # Build the pes binary
make all      # Build pes + test binaries
make clean    # Remove all build artifacts
```

### Author Configuration

PES-VCS reads the author name from the `PES_AUTHOR` environment variable:

```bash
export PES_AUTHOR="Your Name <your.email@example.com>"
```

If unset, it defaults to `"PES User <pes@localhost>"`.

### File Inventory

| File             | Role                                  | Your Task                                   |
| ---------------- | ------------------------------------- | ------------------------------------------- |
| `pes.h`          | Core data structures and constants    | Do not modify                               |
| `object.c`       | Content-addressable object store      | Implement `object_write`, `object_read`     |
| `tree.h`         | Tree object interface                 | Do not modify                               |
| `tree.c`         | Tree serialization and construction   | Implement `tree_parse`, `tree_serialize`, `tree_from_index` |
| `index.h`        | Staging area interface                | Do not modify                               |
| `index.c`        | Staging area (text-based index file)  | Implement `index_load`, `index_save`, `index_add`, `index_status` |
| `commit.h`       | Commit object interface               | Do not modify                               |
| `commit.c`       | Commit creation and history           | Implement `head_read`, `head_update`, `commit_create` |
| `pes.c`          | CLI entry point and command dispatch  | Implement `cmd_commit`                      |
| `test_objects.c`  | Phase 1 test program                  | Do not modify                               |
| `test_tree.c`     | Phase 2 test program                  | Do not modify                               |
| `test_sequence.sh`| End-to-end integration test           | Do not modify                               |
| `Makefile`        | Build system                          | Do not modify                               |

---

## Understanding Git: What You're Building

Before writing code, understand how Git works under the hood. Git is a content-addressable filesystem with a few clever data structures on top. Everything in this lab is based on Git's real design.

### The Big Picture

When you run `git commit`, Git doesn't store "changes" or "diffs." It stores **complete snapshots** of your entire project. Git uses two tricks to make this efficient:

1. **Content-addressable storage:** Every file is stored by the SHA hash of its contents. Same content = same hash = stored only once.
2. **Tree structures:** Directories are stored as "tree" objects that point to file contents, so unchanged files are just pointers to existing data.

```
Your project at commit A:          Your project at commit B:
                                   (only README changed)

    root/                              root/
    ├── README.md  ─────┐              ├── README.md  ─────┐
    ├── src/            │              ├── src/            │
    │   └── main.c ─────┼─┐            │   └── main.c ─────┼─┐
    └── Makefile ───────┼─┼─┐          └── Makefile ───────┼─┼─┐
                        │ │ │                              │ │ │
                        ▼ ▼ ▼                              ▼ ▼ ▼
    Object Store:       ┌─────────────────────────────────────────────┐
                        │  a1b2c3 (README v1)    ← only this is new   │
                        │  d4e5f6 (README v2)                         │
                        │  789abc (main.c)       ← shared by both!    │
                        │  fedcba (Makefile)     ← shared by both!    │
                        └─────────────────────────────────────────────┘
```

### The Three Object Types

#### 1. Blob (Binary Large Object)

A blob is just file contents. No filename, no permissions — just the raw bytes.

```
blob 16\0Hello, World!\n
     ↑    ↑
     │    └── The actual file content
     └─────── Size in bytes
```

The blob is stored at a path determined by its SHA-256 hash. If two files have identical contents, they share one blob.

#### 2. Tree

A tree represents a directory. It's a list of entries, each pointing to a blob (file) or another tree (subdirectory).

```
100644 blob a1b2c3d4... README.md
100755 blob e5f6a7b8... build.sh        ← executable file
040000 tree 9c0d1e2f... src             ← subdirectory
       ↑    ↑           ↑
       │    │           └── name
       │    └── hash of the object
       └─────── mode (permissions + type)
```

Mode values:
- `100644` — regular file, not executable
- `100755` — regular file, executable
- `040000` — directory (tree)

#### 3. Commit

A commit ties everything together. It points to a tree (the project snapshot) and contains metadata.

```
tree 9c0d1e2f3a4b5c6d7e8f9a0b1c2d3e4f5a6b7c8d
parent a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0
author Alice <alice@example.com> 1699900000
committer Alice <alice@example.com> 1699900000

Add new feature
```

The parent pointer creates a linked list of history:

```
    C3 ──────► C2 ──────► C1 ──────► (no parent)
    │          │          │
    ▼          ▼          ▼
  Tree3      Tree2      Tree1
```

### How Objects Connect

```
                    ┌─────────────────────────────────┐
                    │           COMMIT                │
                    │  tree: 7a3f...                  │
                    │  parent: 4b2e...                │
                    │  author: Alice                  │
                    │  message: "Add feature"         │
                    └─────────────┬───────────────────┘
                                  │
                                  ▼
                    ┌─────────────────────────────────┐
                    │         TREE (root)             │
                    │  100644 blob f1a2... README.md  │
                    │  040000 tree 8b3c... src        │
                    │  100644 blob 9d4e... Makefile   │
                    └──────┬──────────┬───────────────┘
                           │          │
              ┌────────────┘          └────────────┐
              ▼                                    ▼
┌─────────────────────────┐          ┌─────────────────────────┐
│      TREE (src)         │          │     BLOB (README.md)    │
│ 100644 blob a5f6 main.c │          │  # My Project           │
└───────────┬─────────────┘          └─────────────────────────┘
            ▼
       ┌────────┐
       │ BLOB   │
       │main.c  │
       └────────┘
```

### References and HEAD

References are files that map human-readable names to commit hashes:

```
.pes/
├── HEAD                    # "ref: refs/heads/main"
└── refs/
    └── heads/
        └── main            # Contains: a1b2c3d4e5f6...
```

**HEAD** points to a branch name. The branch file contains the latest commit hash. When you commit:

1. Git creates the new commit object (pointing to parent)
2. Updates the branch file to contain the new commit's hash
3. HEAD still points to the branch, so it "follows" automatically

```
Before commit:                    After commit:

HEAD ─► main ─► C2 ─► C1         HEAD ─► main ─► C3 ─► C2 ─► C1
```

### The Index (Staging Area)

The index is the "preparation area" for the next commit. It tracks which files are staged.

```
Working Directory          Index               Repository (HEAD)
─────────────────         ─────────           ─────────────────
README.md (modified) ──── pes add ──► README.md (staged)
src/main.c                            src/main.c          ──► Last commit's
Makefile                               Makefile                snapshot
```

The workflow:

1. `pes add file.txt` → computes blob hash, stores blob, updates index
2. `pes commit -m "msg"` → builds tree from index, creates commit, updates branch ref

### Content-Addressable Storage

Objects are named by their content's hash:

```python
# Pseudocode
def store_object(content):
    hash = sha256(content)
    path = f".pes/objects/{hash[0:2]}/{hash[2:]}"
    write_file(path, content)
    return hash
```

This gives us:
- **Deduplication:** Identical files stored once
- **Integrity:** Hash verifies data isn't corrupted
- **Immutability:** Changing content = different hash = different object

Objects are sharded by the first two hex characters to avoid huge directories:

```
.pes/objects/
├── 2f/
│   └── 8a3b5c7d9e...
├── a1/
│   ├── 9c4e6f8a0b...
│   └── b2d4f6a8c0...
└── ff/
    └── 1234567890...
```

### Exploring a Real Git Repository

You can inspect Git's internals yourself:

```bash
mkdir test-repo && cd test-repo && git init
echo "Hello" > hello.txt
git add hello.txt && git commit -m "First commit"

find .git/objects -type f          # See stored objects
git cat-file -t <hash>            # Show type: blob, tree, or commit
git cat-file -p <hash>            # Show contents
cat .git/HEAD                     # See what HEAD points to
cat .git/refs/heads/main          # See branch pointer
```

---

## What You'll Build

PES-VCS implements five commands across four phases:

```
pes init              Create .pes/ repository structure
pes add <file>...     Stage files (hash + update index)
pes status            Show modified/staged/untracked files
pes commit -m <msg>   Create commit from staged files
pes log               Walk and display commit history
```

The `.pes/` directory structure:

```
my_project/
├── .pes/
│   ├── objects/          # Content-addressable blob/tree/commit storage
│   │   ├── 2f/
│   │   │   └── 8a3b...   # Sharded by first 2 hex chars of hash
│   │   └── a1/
│   │       └── 9c4e...
│   ├── refs/
│   │   └── heads/
│   │       └── main      # Branch pointer (file containing commit hash)
│   ├── index             # Staging area (text file)
│   └── HEAD              # Current branch reference
└── (working directory files)
```

### Architecture Overview

```
┌───────────────────────────────────────────────────────────────┐
│                      WORKING DIRECTORY                        │
│                  (actual files you edit)                       │
└───────────────────────────────────────────────────────────────┘
                              │
                        pes add <file>
                              ▼
┌───────────────────────────────────────────────────────────────┐
│                           INDEX                               │
│                (staged changes, ready to commit)              │
│                100644 a1b2c3... src/main.c                    │
└───────────────────────────────────────────────────────────────┘
                              │
                       pes commit -m "msg"
                              ▼
┌───────────────────────────────────────────────────────────────┐
│                       OBJECT STORE                            │
│  ┌───────┐    ┌───────┐    ┌────────┐                         │
│  │ BLOB  │◄───│ TREE  │◄───│ COMMIT │                         │
│  │(file) │    │(dir)  │    │(snap)  │                         │
│  └───────┘    └───────┘    └────────┘                         │
│  Stored at: .pes/objects/XX/YYY...                            │
└───────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌───────────────────────────────────────────────────────────────┐
│                           REFS                                │
│       .pes/refs/heads/main  →  commit hash                    │
│       .pes/HEAD             →  "ref: refs/heads/main"         │
└───────────────────────────────────────────────────────────────┘
```

---

## Phase 1: Object Storage Foundation

**Filesystem Concepts:** Content-addressable storage, directory sharding, atomic writes, hashing for integrity

**Files:** `pes.h` (read), `object.c` (implement `object_write` and `object_read`)

### What to Implement

Open `object.c`. Two functions are marked `// TODO`:

1. **`object_write`** — Stores data in the object store.
   - Prepends a type header (`"blob <size>\0"`, `"tree <size>\0"`, or `"commit <size>\0"`)
   - Computes SHA-256 of the full object (header + data)
   - Writes atomically using the temp-file-then-rename pattern
   - Shards into subdirectories by first 2 hex chars of hash

2. **`object_read`** — Retrieves and verifies data from the object store.
   - Reads the file, parses the header to extract type and size
   - **Verifies integrity** by recomputing the hash and comparing to the filename
   - Returns the data portion (after the `\0`)

Read the detailed step-by-step comments in `object.c` before starting.

### Testing

```bash
make test_objects
./test_objects
```

The test program verifies:
- Blob storage and retrieval (write, read back, compare)
- Deduplication (same content → same hash → stored once)
- Integrity checking (detects corrupted objects)

**📸 Screenshot 1A:** Output of `./test_objects` showing all tests passing.

**📸 Screenshot 1B:** `find .pes/objects -type f` showing the sharded directory structure.

### Analysis Questions

**Q1.1:** Why do we shard objects into subdirectories (`.pes/objects/ab/cdef...`) instead of storing all objects in one flat directory? What filesystem performance problem does this avoid?

**Q1.2:** The `object_write` function must be atomic. Explain what could go wrong if we wrote directly to the final path instead of using write-temp-then-rename. What POSIX guarantee makes `rename()` safe here?

**Q1.3:** Git uses SHA-1 (160 bits). We use SHA-256 (256 bits). Calculate the probability of a hash collision after storing 1 billion objects for each. Why is Git migrating to SHA-256?

---

## Phase 2: Tree Objects

**Filesystem Concepts:** Directory representation, recursive structures, file modes and permissions

**Files:** `tree.h` (read), `tree.c` (implement all TODO functions)

### What to Implement

Open `tree.c`. Three functions are marked `// TODO`:

1. **`tree_parse`** — Parses raw binary tree data into a `Tree` struct.
   - Binary format per entry: `"<mode> <name>\0<32-byte-hash>"`
   - Mode is ASCII octal (e.g., `"100644"`), name is a string, hash is 32 raw bytes

2. **`tree_serialize`** — Serializes a `Tree` struct to binary format.
   - **Must sort entries by name** before serialization (required for deterministic hashing)
   - Same binary format as above

3. **`tree_from_index`** — Builds a tree hierarchy from the index.
   - Handles nested paths: `"src/main.c"` must create a `src` subtree
   - This is what `pes commit` uses to create the snapshot
   - Writes all tree objects to the object store and returns the root hash

### Testing

```bash
make test_tree
./test_tree
```

The test program verifies:
- Serialize → parse roundtrip preserves entries, modes, and hashes
- Deterministic serialization (same entries in any order → identical output)

**📸 Screenshot 2A:** Output of `./test_tree` showing all tests passing.

**📸 Screenshot 2B:** Pick a tree object from `find .pes/objects -type f` and run `xxd .pes/objects/XX/YYY... | head -20` to show the raw binary format.

### Analysis Questions

**Q2.1:** Tree entries must be sorted by name before serialization. Why? What would happen if two developers created the same directory structure but entries were stored in filesystem enumeration order?

**Q2.2:** The mode `100644` vs `100755` only differs in the executable bit. Why does the VCS track this? What problem would occur if it didn't?

**Q2.3:** Git doesn't track empty directories. Based on the tree structure we've implemented, explain why this is a fundamental limitation (not just a design choice).

---

## Phase 3: The Index (Staging Area)

**Filesystem Concepts:** File format design, atomic writes, change detection using metadata

**Files:** `index.h` (read), `index.c` (implement all TODO functions)

### What to Implement

Open `index.c`. Four functions are marked `// TODO`:

1. **`index_load`** — Reads the text-based `.pes/index` file into an `Index` struct.
   - If the file doesn't exist, initializes an empty index (this is not an error)
   - Parses each line: `<mode> <hash-hex> <mtime> <size> <path>`

2. **`index_save`** — Writes the index atomically (temp file + rename).
   - Sorts entries by path before writing
   - Uses `fsync()` on the temp file before renaming

3. **`index_add`** — Stages a file: reads it, writes blob to object store, updates index entry.
   - Use the provided `index_find` to check for an existing entry

4. **`index_status`** — Prints the status of the working directory (staged changes, unstaged changes, untracked files).

`index_find` and `index_remove` are already implemented for you — read them to understand the index data structure before starting.

#### Expected Output of `pes status`

```
Staged changes:
    new file:   hello.txt
    modified:   src/main.c

Unstaged changes:
    modified:   README.md
    deleted:    old_file.txt

Untracked files:
    notes.txt
```

If a section has no entries, print the header followed by `(nothing to show)`.

### Testing

```bash
make pes
./pes init
echo "hello" > file1.txt
echo "world" > file2.txt
./pes add file1.txt file2.txt
./pes status
cat .pes/index    # Human-readable text format
```

**📸 Screenshot 3A:** Run `./pes init`, `./pes add file1.txt file2.txt`, `./pes status` — show the output.

**📸 Screenshot 3B:** `cat .pes/index` showing the text-format index with your entries.

### Analysis Questions

**Q3.1:** The index stores `mtime` and `size` for each entry. How does this optimize `pes status`? What's the worst-case scenario where this optimization fails (i.e., a file is modified but mtime+size stay the same)?

**Q3.2:** Why must index entries be sorted by path before saving? (Hint: same reason as tree entries — what happens if they aren't?)

**Q3.3:** We use the write-temp-then-rename pattern for saving the index. Why is this critical? What state would the repository be in if we wrote directly to `.pes/index` and crashed halfway through?

---

## Phase 4: Commits and History

**Filesystem Concepts:** Linked structures on disk, reference files, atomic pointer updates

**Files:** `commit.h` (read), `commit.c` (implement all TODO functions), `pes.c` (implement `cmd_commit`)

### What to Implement

Open `commit.c`. Three functions are marked `// TODO`:

1. **`head_read`** — Reads the commit hash that HEAD points to. Follows symbolic refs (HEAD → branch file → commit hash).

2. **`head_update`** — Atomically updates the current branch ref to a new commit hash. This is the "pointer swing" — the single atomic operation that publishes a commit.

3. **`commit_create`** — The main commit function:
   - Builds a tree from the index using `tree_from_index()` (**not** from the working directory — commits snapshot the staged state)
   - Reads current HEAD as the parent (may not exist for first commit)
   - Gets the author string from `pes_author()` (defined in `pes.h`)
   - Writes the commit object, then updates HEAD

`commit_parse`, `commit_serialize`, and `commit_walk` are already implemented — read them to understand the commit format before writing `commit_create`.

Also implement **`cmd_commit`** in `pes.c`:
- Parse `-m <message>` from command-line arguments
- If `-m` is missing, print: `error: commit requires a message (-m "message")`
- On success, print: `Committed: <first-12-hex-chars>... <message>`

The commit text format is specified in the comment at the top of `commit.c`.

### Testing

```bash
./pes init
echo "Hello" > hello.txt
./pes add hello.txt
./pes commit -m "Initial commit"

echo "World" >> hello.txt
./pes add hello.txt
./pes commit -m "Add world"

echo "Goodbye" > bye.txt
./pes add bye.txt
./pes commit -m "Add farewell"

./pes log
```

You can also run the full integration test:

```bash
make test-integration
```

**📸 Screenshot 4A:** Output of `./pes log` showing three commits with hashes, authors, timestamps, and messages.

**📸 Screenshot 4B:** `find .pes -type f | sort` showing object store growth after three commits.

**📸 Screenshot 4C:** `cat .pes/refs/heads/main` and `cat .pes/HEAD` showing the reference chain.

### Analysis Questions

**Q4.1:** The reference file `.pes/refs/heads/main` contains just a 64-character hex hash. Why is updating this file the "commit point" — the moment when a commit becomes part of history?

**Q4.2:** If power fails after writing the commit object but before updating the ref, what state is the repository in? Is the commit recoverable? Why is this safe?

**Q4.3:** We update refs using write-temp-rename. What would happen if we used `write()` to overwrite in place and crashed after writing 32 of the 64 hex characters?

---

## Analysis-Only Questions

The following questions cover filesystem concepts beyond the implementation scope of this lab. Answer them in writing — no code required.

### Branching and Checkout

**Q5.1:** A branch in Git is just a file in `.git/refs/heads/` containing a commit hash. Creating a branch is creating a file. Given this, how would you implement `pes checkout <branch>` — what files need to change in `.pes/`, and what must happen to the working directory? What makes this operation complex?

**Q5.2:** When switching branches, the working directory must be updated to match the target branch's tree. If the user has uncommitted changes to a tracked file, and that file differs between branches, checkout must refuse. Describe how you would detect this "dirty working directory" conflict using only the index and the object store.

**Q5.3:** "Detached HEAD" means HEAD contains a commit hash directly instead of a branch reference. What happens if you make commits in this state? How could a user recover those commits?

### Garbage Collection and Space Reclamation

**Q6.1:** Over time, the object store accumulates unreachable objects — blobs, trees, or commits that no branch points to (directly or transitively). Describe an algorithm to find and delete these objects. What data structure would you use to track "reachable" hashes efficiently? For a repository with 100,000 commits and 50 branches, estimate how many objects you'd need to visit.

**Q6.2:** Why is it dangerous to run garbage collection concurrently with a commit operation? Describe a race condition where GC could delete an object that a concurrent commit is about to reference. How does Git's real GC avoid this?

---

## Submission Checklist

### Screenshots Required

| Phase | ID  | What to Capture                                                 |
| ----- | --- | --------------------------------------------------------------- |
| 1     | 1A  | `./test_objects` output showing all tests passing               |
| 1     | 1B  | `find .pes/objects -type f` showing sharded directory structure |
| 2     | 2A  | `./test_tree` output showing all tests passing                  |
| 2     | 2B  | `xxd` of a raw tree object (first 20 lines)                    |
| 3     | 3A  | `pes init` → `pes add` → `pes status` sequence                 |
| 3     | 3B  | `cat .pes/index` showing the text-format index                  |
| 4     | 4A  | `pes log` output with three commits                            |
| 4     | 4B  | `find .pes -type f \| sort` showing object growth              |
| 4     | 4C  | `cat .pes/refs/heads/main` and `cat .pes/HEAD`                 |
| Final | --  | Full integration test (`make test-integration`)                 |

### Code Files Required (5 files)

| File           | Description                              |
| -------------- | ---------------------------------------- |
| `object.c`     | Object store implementation              |
| `tree.c`       | Tree serialization and construction      |
| `index.c`      | Staging area implementation              |
| `commit.c`     | Commit creation and history walking      |
| `pes.c`        | CLI entry point with `cmd_commit`        |

### Analysis Questions (written answers)

| Section                    | Questions            |
| -------------------------- | -------------------- |
| Phase 1: Object Store      | Q1.1, Q1.2, Q1.3    |
| Phase 2: Trees             | Q2.1, Q2.2, Q2.3    |
| Phase 3: Index             | Q3.1, Q3.2, Q3.3    |
| Phase 4: Commits           | Q4.1, Q4.2, Q4.3    |
| Branching (analysis-only)  | Q5.1, Q5.2, Q5.3    |
| GC (analysis-only)         | Q6.1, Q6.2          |

---

## Filesystem Concept Coverage

| Concept                     | Where It Appears                                |
| --------------------------- | ----------------------------------------------- |
| Content-addressable storage | Phase 1 — object store                          |
| Directory sharding          | Phase 1 — `.pes/objects/XX/`                    |
| Atomic writes               | Phases 1, 3, 4 — all writes use temp+rename     |
| Hashing for integrity       | Phase 1 — corruption detection                  |
| Directory representation    | Phase 2 — tree objects                          |
| File modes and permissions  | Phase 2 — tree entries (100644, 100755, 040000) |
| File format design          | Phase 3 — index text format                     |
| Change detection            | Phase 3 — mtime+size optimization in status     |
| Reference files             | Phase 4 — branches and HEAD                     |
| Linked structures on disk   | Phase 4 — commit parent chain                   |
| Atomic pointer updates      | Phase 4 — commit publication via ref update     |
| Working directory sync      | Q5.1 — checkout (analysis)                      |
| Reachability / free-space   | Q6.1 — garbage collection (analysis)            |

---

## Further Reading

- **Git Internals** (Pro Git book): https://git-scm.com/book/en/v2/Git-Internals-Plumbing-and-Porcelain
- **Git from the inside out**: https://codewords.recurse.com/issues/two/git-from-the-inside-out
- **The Git Parable**: https://tom.preston-werner.com/2009/05/19/the-git-parable.html
