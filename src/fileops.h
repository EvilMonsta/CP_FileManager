#ifndef FILEOPS_H
#define FILEOPS_H
#include <limits.h>
#define MAX_FILES 1024
#define NAME_LEN 256

typedef struct {
    char name[NAME_LEN];         
    int is_dir;                 
} FileEntry;

typedef struct {
    char path[PATH_MAX];       
    FileEntry files[MAX_FILES]; 
    int file_count;            
    int selected;            
    int offset;             
} Panel;

void load_directory(Panel* panel);
void delete_entry(Panel* panel);

#endif