// index.c — Staging area implementation
//
// Text format of .pes/index (one entry per line, sorted by path):
//
//   <mode-octal> <64-char-hex-hash> <mtime-seconds> <size> <path>
//
// Example:
//   100644 a1b2c3d4e5f6...  1699900000 42 README.md
//   100644 f7e8d9c0b1a2...  1699900100 128 src/main.c
//
// This is intentionally a simple text format. No magic numbers, no
// binary parsing. The focus is on the staging area CONCEPT (tracking
// what will go into the next commit) and ATOMIC WRITES (temp+rename),
// not on binary format gymnastics.
//
// PROVIDED functions: index_find, index_remove
// TODO functions:     index_load, index_save, index_add, index_status

#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

// ─── PROVIDED ────────────────────────────────────────────────────────────────

// Find an index entry by path (linear scan).
IndexEntry* index_find(Index *index, const char *path) {
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0)
            return &index->entries[i];
    }
    return NULL;
}

// Remove a file from the index.
// Returns 0 on success, -1 if path not in index.
int index_remove(Index *index, const char *path) {
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0) {
            int remaining = index->count - i - 1;
            if (remaining > 0)
                memmove(&index->entries[i], &index->entries[i + 1],
                        remaining * sizeof(IndexEntry));
            index->count--;
            return index_save(index);
        }
    }
    fprintf(stderr, "error: '%s' is not in the index\n", path);
    return -1;
}

// ─── TODO: Implement these ───────────────────────────────────────────────────

// Load the index from .pes/index.
//
// Steps:
//   1. If .pes/index does not exist, set index->count = 0 and return 0
//   2. Open the file for reading (fopen with "r")
//   3. For each line, parse the fields:
//      - Use fscanf or sscanf to read: mode, hex-hash, mtime, size, path
//      - Convert the 64-char hex hash to an ObjectID using hex_to_hash()
//   4. Populate index->entries and index->count
//
// Returns 0 on success, -1 on error.
int index_load(Index *index) {
    // TODO: Implement
    (void)index;
    return -1;
}

// Save the index to .pes/index atomically.
//
// Steps:
//   1. Sort entries by path (use qsort with strcmp on the path field)
//   2. Open a temporary file for writing (.pes/index.tmp)
//   3. For each entry, write one line:
//      "<mode> <64-char-hex-hash> <mtime_sec> <size> <path>\n"
//      - Convert ObjectID to hex using hash_to_hex()
//   4. fflush() and fsync() the temp file to ensure data reaches disk
//   5. fclose() the temp file
//   6. rename(".pes/index.tmp", ".pes/index") — atomic replacement
//
// The rename() call is the key filesystem concept here: it is atomic
// on POSIX systems, meaning the index file is never in a half-written
// state even if the system crashes.
//
// Returns 0 on success, -1 on error.
int index_save(const Index *index) {
    // TODO: Implement
    (void)index;
    return -1;
}

// Stage a file for the next commit.
//
// Steps:
//   1. Open and read the file at `path`
//   2. Write the file contents as a blob: object_write(OBJ_BLOB, ...)
//   3. Stat the file to get mode, mtime, and size
//      - Use stat() or lstat()
//      - mtime_sec = st.st_mtime
//      - size = st.st_size
//      - mode: use 0100755 if executable (st_mode & S_IXUSR), else 0100644
//   4. Search the index for an existing entry with this path (index_find)
//      - If found: update its hash, mode, mtime, and size
//      - If not found: append a new entry (check count < MAX_INDEX_ENTRIES)
//   5. Save the index to disk (index_save)
//
// Returns 0 on success, -1 on error (file not found, etc.).
int index_add(Index *index, const char *path) {
    // TODO: Implement
    (void)index; (void)path;
    return -1;
}

// Print the status of the working directory.
//
// This involves THREE comparisons:
//
// 1. Index vs HEAD (staged changes):
//    - Load the HEAD commit's tree (if any commits exist)
//    - For each index entry, check if it exists in HEAD's tree with the same hash
//    - New in index but not in HEAD:       "new file:   <path>"
//    - In both but different hash:          "modified:   <path>"
//
// 2. Working directory vs index (unstaged changes):
//    - For each index entry, check the working directory file
//    - If file is missing:                  "deleted:    <path>"
//    - If file's mtime or size changed, recompute its hash:
//      - If hash differs from index:        "modified:   <path>"
//    - (If mtime+size unchanged, skip — assume file is unmodified)
//
// 3. Untracked files:
//    - Scan the working directory (skip .pes/)
//    - Any file not in the index:           "<path>"
//
// Expected output:
//   Staged changes:
//       new file:   hello.txt
//
//   Unstaged changes:
//       modified:   README.md
//
//   Untracked files:
//       notes.txt
//
// If a section has no entries, print the header followed by
//   (nothing to show)
//
// Returns 0.
int index_status(const Index *index) {
    // TODO: Implement
    (void)index;
    return -1;
}
