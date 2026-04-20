// tree.c — Tree object serialization and construction
//
// PROVIDED functions: get_file_mode, tree_parse, tree_serialize
// TODO functions:     tree_from_index
//
// Binary tree format (per entry, concatenated with no separators):
//   "<mode-as-ascii-octal> <name>\0<32-byte-binary-hash>"
//
// Example single entry (conceptual):
//   "100644 hello.txt\0" followed by 32 raw bytes of SHA-256

#include "tree.h"
#include "index.h" // Required for Index and index_load
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

__attribute__((weak)) int index_load(Index *index) {
    (void)index;
    return -1; 
}

// ─── Mode Constants ─────────────────────────────────────────────────────────

#define MODE_FILE      0100644
#define MODE_EXEC      0100755
#define MODE_DIR       0040000

// External declaration for object_write (implemented in object.c)
extern int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out);

// ─── PROVIDED ───────────────────────────────────────────────────────────────

// Determine the object mode for a filesystem path.
uint32_t get_file_mode(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) return 0;

    if (S_ISDIR(st.st_mode))  return MODE_DIR;
    if (st.st_mode & S_IXUSR) return MODE_EXEC;
    return MODE_FILE;
}

// Parse binary tree data into a Tree struct safely.
// Returns 0 on success, -1 on parse error.
int tree_parse(const void *data, size_t len, Tree *tree_out) {
    tree_out->count = 0;
    const uint8_t *ptr = (const uint8_t *)data;
    const uint8_t *end = ptr + len;

    while (ptr < end && tree_out->count < MAX_TREE_ENTRIES) {
        TreeEntry *entry = &tree_out->entries[tree_out->count];

        // 1. Safely find the space character for the mode
        const uint8_t *space = memchr(ptr, ' ', end - ptr);
        if (!space) return -1; // Malformed data

        // Parse mode into an isolated buffer
        char mode_str[16] = {0};
        size_t mode_len = space - ptr;
        if (mode_len >= sizeof(mode_str)) return -1;
        memcpy(mode_str, ptr, mode_len);
        entry->mode = strtol(mode_str, NULL, 8);

        ptr = space + 1; // Skip space

        // 2. Safely find the null terminator for the name
        const uint8_t *null_byte = memchr(ptr, '\0', end - ptr);
        if (!null_byte) return -1; // Malformed data

        size_t name_len = null_byte - ptr;
        if (name_len >= sizeof(entry->name)) return -1;
        memcpy(entry->name, ptr, name_len);
        entry->name[name_len] = '\0'; // Ensure null-terminated

        ptr = null_byte + 1; // Skip null byte

        // 3. Read the 32-byte binary hash
        if (ptr + HASH_SIZE > end) return -1; 
        memcpy(entry->hash.hash, ptr, HASH_SIZE);
        ptr += HASH_SIZE;

        tree_out->count++;
    }
    return 0;
}

// Helper for qsort to ensure consistent tree hashing
static int compare_tree_entries(const void *a, const void *b) {
    return strcmp(((const TreeEntry *)a)->name, ((const TreeEntry *)b)->name);
}

// Serialize a Tree struct into binary format for storage.
// Caller must free(*data_out).
// Returns 0 on success, -1 on error.
int tree_serialize(const Tree *tree, void **data_out, size_t *len_out) {
    // Estimate max size: (6 bytes mode + 1 byte space + 256 bytes name + 1 byte null + 32 bytes hash) per entry
    size_t max_size = tree->count * 296; 
    uint8_t *buffer = malloc(max_size);
    if (!buffer) return -1;

    // Create a mutable copy to sort entries (Git requirement)
    Tree sorted_tree = *tree;
    qsort(sorted_tree.entries, sorted_tree.count, sizeof(TreeEntry), compare_tree_entries);

    size_t offset = 0;
    for (int i = 0; i < sorted_tree.count; i++) {
        const TreeEntry *entry = &sorted_tree.entries[i];
        
        // Write mode and name (%o writes octal correctly for Git standards)
        int written = sprintf((char *)buffer + offset, "%o %s", entry->mode, entry->name);
        offset += written + 1; // +1 to step over the null terminator written by sprintf
        
        // Write binary hash
        memcpy(buffer + offset, entry->hash.hash, HASH_SIZE);
        offset += HASH_SIZE;
    }

    *data_out = buffer;
    *len_out = offset;
    return 0;
}

// ─── TODO: Implement these ──────────────────────────────────────────────────

// Recursive helper function to group paths and build nested Tree structures
static int build_tree_level(IndexEntry *entries, int count, int depth, ObjectID *out_id) {
    Tree tree;
    tree.count = 0;
    int i = 0;

    while (i < count) {
        // Adjust the path pointer based on our current depth
        const char *current_path = entries[i].path + depth;
        char *slash = strchr(current_path, '/');

        if (slash) {
            // It's a directory. Group all contiguous entries that belong to this subdirectory.
            int dir_len = slash - current_path;
            int j = i;
            
            while (j < count) {
                const char *next_path = entries[j].path + depth;
                if (strncmp(next_path, current_path, dir_len) == 0 && next_path[dir_len] == '/') {
                    j++;
                } else {
                    break;
                }
            }

            // Recursively build the sub-tree
            ObjectID sub_tree_id;
            if (build_tree_level(&entries[i], j - i, depth + dir_len + 1, &sub_tree_id) != 0) {
                return -1;
            }

            // Add the directory entry to the current tree
            TreeEntry *te = &tree.entries[tree.count++];
            te->mode = MODE_DIR;
            strncpy(te->name, current_path, dir_len);
            te->name[dir_len] = '\0';
            te->hash = sub_tree_id;

            // Advance the outer loop past all entries grouped into this subdirectory
            i = j; 
        } else {
            // It's a file at the current directory level
            TreeEntry *te = &tree.entries[tree.count++];
            te->mode = entries[i].mode;
            strcpy(te->name, current_path);
            te->hash = entries[i].hash;
            i++;
        }
    }

    // Serialize and write the constructed tree object to the object store
    void *data;
    size_t len;
    if (tree_serialize(&tree, &data, &len) != 0) return -1;
    
    int ret = object_write(OBJ_TREE, data, len, out_id);
    free(data);
    return ret;
}

// Build a tree hierarchy from the current index and write all tree
// objects to the object store.
int tree_from_index(ObjectID *id_out) {
    Index index;
    if (index_load(&index) != 0) {
        return -1;
    }
    
    // Handle the edge case of an empty commit / empty staging area
    if (index.count == 0) {
        Tree empty_tree;
        empty_tree.count = 0;
        
        void *data; 
        size_t len;
        if (tree_serialize(&empty_tree, &data, &len) != 0) return -1;
        
        int ret = object_write(OBJ_TREE, data, len, id_out);
        free(data);
        return ret;
    }

    return build_tree_level(index.entries, index.count, 0, id_out);
}
