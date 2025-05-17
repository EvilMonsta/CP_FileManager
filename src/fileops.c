#include "fileops.h"
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

static int compare_entries(const void* a, const void* b) {
    const FileEntry* fa = (const FileEntry*)a;
    const FileEntry* fb = (const FileEntry*)b;

    if (strcmp(fa->name, ".") == 0) return -1;
    if (strcmp(fb->name, ".") == 0) return 1;
    if (strcmp(fa->name, "..") == 0) return -1;
    if (strcmp(fb->name, "..") == 0) return 1;

    if (fa->is_dir != fb->is_dir)
        return fb->is_dir - fa->is_dir;

    return strcasecmp(fa->name, fb->name);
}

void load_directory(Panel* panel) {
    DIR* dir = opendir(panel->path);
    if (!dir) {
        panel->file_count = 0;
        return;
    }

    panel->file_count = 0;
    struct dirent* entry;

    if (strcmp(panel->path, "/") != 0) {
        strncpy(panel->files[panel->file_count].name, "..", NAME_LEN);
        panel->files[panel->file_count].is_dir = 1;
        panel->file_count++;
    }
    
    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
    
        if(strcmp(panel->path, "/") == 0 &&
            (!strcmp(entry->d_name, "root") ||
             !strcmp(entry->d_name, "proc") ||
             !strcmp(entry->d_name, "sys") ||
             !strcmp(entry->d_name, "run") ||
             !strcmp(entry->d_name, "sbin") ||
             !strcmp(entry->d_name, "bin") ||
             !strcmp(entry->d_name, "dev"))) {
            continue;
        }

        if (panel->file_count >= MAX_FILES)
            break;        
        FileEntry* fe = &panel->files[panel->file_count];

        strncpy(fe->name, entry->d_name, NAME_LEN - 1);
        fe->name[NAME_LEN - 1] = '\0';

        char full_path[PATH_MAX];
        int res = snprintf(full_path, sizeof(full_path), "%s/%s", panel->path, entry->d_name);
        if (res < 0) {
            fprintf(stderr, "Ошибка форматирования пути.\n");
            return;
        } else if (res >= (int)sizeof(full_path)) {
            fprintf(stderr, "Слишком длинный путь, произошло усечение.\n");
            return;
        }
        
        struct stat st;
        fe->is_dir = (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode));

        panel->file_count++;
    }

    closedir(dir);

    qsort(panel->files, panel->file_count, sizeof(FileEntry), compare_entries);

    panel->selected = 0;
    panel->offset = 0;
}

static int delete_file(const char* path) {
    return unlink(path);
}

static int delete_directory(const char* path) {
    DIR* dir = opendir(path);
    if (!dir) return -1;
    struct dirent* entry;
    char full_path[PATH_MAX];

    while ((entry = readdir(dir))) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) delete_directory(full_path);
            else delete_file(full_path);
        }
    }
    closedir(dir);
    return rmdir(path);
}

void delete_entry(Panel* panel) {
    if (panel->file_count == 0) return;

    FileEntry* entry = &panel->files[panel->selected];
    if (!strcmp(entry->name, ".") || !strcmp(entry->name, "..")) return;

    char full_path[PATH_MAX];
    int res = snprintf(full_path, sizeof(full_path), "%s/%s", panel->path, entry->name);
    if (res < 0) {
        fprintf(stderr, "Ошибка форматирования пути.\n");
        return;
    } else if (res >= (int)sizeof(full_path)) {
        fprintf(stderr, "Слишком длинный путь, произошло усечение.\n");
        return;
    }
    
    
    if (entry->is_dir) delete_directory(full_path);
    else delete_file(full_path);

    load_directory(panel);
}
