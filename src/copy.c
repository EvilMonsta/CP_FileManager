#include "copy.h"
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static bool copy_file(const char* src, const char* dest) {
    FILE* in = fopen(src, "rb");
    if (!in) return false;

    FILE* out = fopen(dest, "wb");
    if (!out) {
        fclose(in);
        return false;
    }

    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), in)) > 0) {
        fwrite(buffer, 1, bytes, out);
    }

    fclose(in);
    fclose(out);
    return true;
}

static bool copy_dir(const char* src, const char* dest) {
    struct stat st;
    if (stat(dest, &st) != 0) {
        if (mkdir(dest, 0755) != 0) return false;
    }

    DIR* dir = opendir(src);
    if (!dir) return false;

    struct dirent* entry;
    char src_path[PATH_MAX];
    char dest_path[PATH_MAX];

    while ((entry = readdir(dir))) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;

        snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest, entry->d_name);

        struct stat entry_st;
        if (stat(src_path, &entry_st) == 0) {
            if (S_ISDIR(entry_st.st_mode)) {
                if (!copy_dir(src_path, dest_path)) {
                    closedir(dir);
                    return false;
                }
            } else {
                if (!copy_file(src_path, dest_path)) {
                    closedir(dir);
                    return false;
                }
            }
        }
    }

    closedir(dir);
    return true;
}

bool copy_entry(const char* src, const char* dest) {
    struct stat st;
    if (stat(src, &st) != 0) return false;

    if (S_ISDIR(st.st_mode)) return copy_dir(src, dest);
    else return copy_file(src, dest);
}
