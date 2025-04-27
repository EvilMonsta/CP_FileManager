#include "search.h"
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>

static int is_system_dir(const char* path) {
    return (strncmp(path, "/proc", 5) == 0 || 
            strncmp(path, "/sys", 4) == 0 ||
            strncmp(path, "/dev", 4) == 0 ||
            strncmp(path, "/run", 4) == 0);
}

static void search_recursive(const char* path, const char* query, SearchResult* result, int depth, int* dirs_scanned) {
    if (result->count >= MAX_RESULTS || depth > MAX_DEPTH || *dirs_scanned >= MAX_DIRS_SCANNED) return;
    if (is_system_dir(path)) return;

    DIR* dir = opendir(path);
    if (!dir) return;

    (*dirs_scanned)++;

    struct dirent* entry;
    while ((entry = readdir(dir))) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (strstr(entry->d_name, query)) {
            strncpy(result->results[result->count], full_path, PATH_MAX - 1);
            result->results[result->count][PATH_MAX - 1] = '\0';
            result->count++;
            if (result->count >= MAX_RESULTS) break;
        }

        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            search_recursive(full_path, query, result, depth + 1, dirs_scanned);
        }

        if (*dirs_scanned >= MAX_DIRS_SCANNED) break;
    }
    closedir(dir);
}

void search_files(const char* base_path, const char* query, SearchResult* result) {
    result->count = 0;
    result->selected = 0;
    result->active = 1;
    int dirs_scanned = 0;
    search_recursive(base_path, query, result, 0, &dirs_scanned);

    if (dirs_scanned >= MAX_DIRS_SCANNED) {
        printf("Alert: scan limit (%d dirs).\n", MAX_DIRS_SCANNED);
    }
}