#include "search.h"
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>

#include <limits.h>
#include <stdlib.h>

static int is_system_dir(const char* path) {
    char resolved[PATH_MAX];
    if (!realpath(path, resolved)) return 1; 

    return (strncmp(resolved, "/proc", 5) == 0 ||
            strncmp(resolved, "/sys", 4) == 0 ||
            strncmp(resolved, "/dev", 4) == 0 ||
            strncmp(resolved, "/run", 4) == 0 ||
            strncmp(resolved, "/sbin", 5) == 0 ||
            strncmp(resolved, "/bin", 4) == 0 ||
            strncmp(resolved, "/root", 5) == 0);
}



static void search_recursive(const char* path, const char* query, SearchResult* result, int depth, int* dirs_scanned) {
    if (!path || !query || !result) return;
    if (result->count >= MAX_RESULTS || depth > MAX_DEPTH || *dirs_scanned >= MAX_DIRS_SCANNED) return;
    if (is_system_dir(path)) return;

    DIR* dir = opendir(path);
    if (!dir) return;

    (*dirs_scanned)++;

    struct dirent* entry;
    while ((entry = readdir(dir))) {
        if (!entry) continue;
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;

        char full_path[PATH_MAX];
        int res = snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        if (res < 0 || res >= (int)sizeof(full_path)) continue;

        struct stat st;
        if (lstat(full_path, &st) != 0) continue;

        if (S_ISLNK(st.st_mode)) continue; 

        if (result->count < MAX_RESULTS && strstr(entry->d_name, query)) {
            strncpy(result->results[result->count], full_path, PATH_MAX - 1);
            result->results[result->count][PATH_MAX - 1] = '\0';
            result->count++;
        }

        if (S_ISDIR(st.st_mode)) {
            search_recursive(full_path, query, result, depth + 1, dirs_scanned);
        }

        if (*dirs_scanned >= MAX_DIRS_SCANNED || result->count >= MAX_RESULTS) break;
    }

    closedir(dir);
}


void search_files(const char* base_path, const char* query, SearchResult* result) {
    if (!base_path || !query || !result) return;

    result->count = 0;
    result->selected = 0;
    result->active = 1;
    result->limit_reached = 0;

    int dirs_scanned = 0;
    search_recursive(base_path, query, result, 0, &dirs_scanned);
    result->limit_reached = (dirs_scanned >= MAX_DIRS_SCANNED);
}