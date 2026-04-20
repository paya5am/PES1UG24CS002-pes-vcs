// tree.c — Tree object serialization and construction
//
// PROVIDED functions: get_file_mode
// TODO functions:     tree_parse, tree_serialize, tree_from_index
//
// Binary tree format (per entry, concatenated with no separators):
//   "<mode-as-ascii-octal> <name>\0<32-byte-binary-hash>"
//
// Example single entry (conceptual):
//   "100644 hello.txt\0" followed by 32 raw bytes of SHA-256

#include "tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

// ─── Mode Constants ─────────────────────────────────────────────────────────

#define MODE_FILE      0100644
#define MODE_EXEC      0100755
#define MODE_DIR       0040000

// ─── PROVIDED ───────────────────────────────────────────────────────────────

// Determine the object mode for a filesystem path.
uint32_t get_file_mode(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) return 0;

    if (S_ISDIR(st.st_mode))  return MODE_DIR;
    if (st.st_mode & S_IXUSR) return MODE_EXEC;
    return MODE_FILE;
}

// ─── TODO: Implement these ──────────────────────────────────────────────────

// Parse binary tree data into a Tree struct.
//
// The input `data` contains concatenated entries, each formatted as:
//   "<mode> <name>\0<32-byte-hash>"
// where <mode> is an ASCII octal string (e.g., "100644").
//
// Steps:
//   1. Start a pointer at the beginning of data
//   2. For each entry:
//      a. Read the mode string up to the space character
//      b. Read the name from after the space up to the null byte
//      c. Read the next 32 bytes as the raw binary hash
//      d. Store in tree_out->entries[tree_out->count++]
//   3. Stop when you've consumed all `len` bytes
//
// Returns 0 on success, -1 on parse error.
int tree_parse(const void *data, size_t len, Tree *tree_out) {
    // TODO: Implement
    (void)data; (void)len; (void)tree_out;
    return -1;
}

// Serialize a Tree struct into binary format for storage.
//
// Steps:
//   1. Sort entries by name (required for deterministic hashing —
//      same directory contents must always produce the same tree hash)
//   2. Calculate total output size
//   3. Allocate output buffer
//   4. For each entry, write: "<mode> <name>\0<32-byte-hash>"
//   5. Set *data_out and *len_out
//
// Caller must free(*data_out).
// Returns 0 on success, -1 on error.
int tree_serialize(const Tree *tree, void **data_out, size_t *len_out) {
    // TODO: Implement
    (void)tree; (void)data_out; (void)len_out;
    return -1;
}

// Build a tree hierarchy from the current index and write all tree
// objects to the object store.
//
// The index contains flat paths like:
//   "README.md"
//   "src/main.c"
//   "src/util/helper.c"
//
// You must construct a tree hierarchy:
//   root tree:
//     100644 blob <hash> README.md
//     040000 tree <hash> src
//   src tree:
//     100644 blob <hash> main.c
//     040000 tree <hash> util
//   util tree:
//     100644 blob <hash> helper.c
//
// Steps:
//   1. Load the index (use index_load from index.h)
//   2. Group entries by their top-level directory component
//   3. For files at the current level, add blob entries to the tree
//   4. For files in subdirectories, recursively build subtrees
//   5. Serialize and write each tree object using object_write(OBJ_TREE, ...)
//   6. Return the root tree's hash in *id_out
//
// Returns 0 on success, -1 on error.
int tree_from_index(ObjectID *id_out) {
    // TODO: Implement
    (void)id_out;
    return -1;
}
