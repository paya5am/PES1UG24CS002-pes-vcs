// pes.c — CLI entry point for PES-VCS
//
// This file dispatches user commands to the appropriate module functions.
//
// PROVIDED: cmd_init, cmd_add, cmd_status, cmd_log, main
// TODO:     cmd_commit (thin wrapper — the real logic is in commit.c)

#include "pes.h"
#include "tree.h"
#include "index.h"
#include "commit.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

// ─── PROVIDED ───────────────────────────────────────────────────────────────

// Helper: create a directory and all parent directories (like "mkdir -p").
static int mkdirs(const char *path, mode_t mode) {
    char tmp[512];
    char *p = NULL;
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, mode) != 0 && errno != EEXIST) return -1;
            *p = '/';
        }
    }
    if (mkdir(tmp, mode) != 0 && errno != EEXIST) return -1;
    return 0;
}

void cmd_init(void) {
    if (mkdirs(PES_DIR, 0755) != 0)    { perror("mkdir .pes"); return; }
    if (mkdirs(OBJECTS_DIR, 0755) != 0) { perror("mkdir objects"); return; }
    if (mkdirs(REFS_DIR, 0755) != 0)    { perror("mkdir refs"); return; }

    FILE *f = fopen(HEAD_FILE, "w");
    if (!f) { perror("fopen HEAD"); return; }
    fprintf(f, "ref: refs/heads/main\n");
    fclose(f);

    printf("Initialized empty PES repository in .pes/\n");
}

void cmd_add(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: pes add <file>...\n");
        return;
    }

    Index index;
    if (index_load(&index) != 0) {
        fprintf(stderr, "error: failed to load index\n");
        return;
    }

    for (int i = 2; i < argc; i++) {
        if (index_add(&index, argv[i]) == 0) {
            printf("Added: %s\n", argv[i]);
        } else {
            fprintf(stderr, "Failed to add: %s\n", argv[i]);
        }
    }
}

void cmd_status(void) {
    Index index;
    if (index_load(&index) != 0) {
        fprintf(stderr, "error: failed to load index\n");
        return;
    }
    index_status(&index);
}

static void log_callback(const ObjectID *id, const Commit *commit, void *ctx) {
    (void)ctx;
    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(id, hex);

    time_t t = (time_t)commit->timestamp;
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&t));

    printf("\033[33mcommit %s\033[0m\n", hex);
    printf("Author: %s\n", commit->author);
    printf("Date:   %s\n", timebuf);
    printf("\n    %s\n\n", commit->message);
}

void cmd_log(void) {
    if (commit_walk(log_callback, NULL) != 0) {
        fprintf(stderr, "error: no commits yet\n");
    }
}

// ─── TODO: Implement this command wrapper ───────────────────────────────────

// Parse "-m <message>" from argv and call commit_create().
//
// Usage: pes commit -m "commit message"
//
// Steps:
//   1. Search argv for "-m". The next argument is the message string.
//      If "-m" is missing or has no argument after it, print:
//        "error: commit requires a message (-m \"message\")"
//      and return.
//   2. Call commit_create(message, &id)
//   3. On success, print: "Committed: <first-12-hex-chars>... <message>"
//   4. On failure, print: "error: commit failed"
void cmd_commit(int argc, char *argv[]) {
    // TODO: Implement
    (void)argc; (void)argv;
    fprintf(stderr, "error: commit not yet implemented\n");
}

// ─── PROVIDED: Command dispatch ─────────────────────────────────────────────

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: pes <command> [args]\n");
        fprintf(stderr, "\nCommands:\n");
        fprintf(stderr, "  init              Create a new PES repository\n");
        fprintf(stderr, "  add <file>...     Stage files for commit\n");
        fprintf(stderr, "  status            Show working directory status\n");
        fprintf(stderr, "  commit -m <msg>   Create a commit from staged files\n");
        fprintf(stderr, "  log               Show commit history\n");
        return 1;
    }

    const char *cmd = argv[1];

    if      (strcmp(cmd, "init") == 0)     cmd_init();
    else if (strcmp(cmd, "add") == 0)      cmd_add(argc, argv);
    else if (strcmp(cmd, "status") == 0)   cmd_status();
    else if (strcmp(cmd, "commit") == 0)   cmd_commit(argc, argv);
    else if (strcmp(cmd, "log") == 0)      cmd_log();
    else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        fprintf(stderr, "Run 'pes' with no arguments for usage.\n");
        return 1;
    }

    return 0;
}
