#ifndef FILEOPS_H
#define FILEOPS_H
#include <limits.h>
#define MAX_FILES 1024
#define NAME_LEN 256

typedef struct {
    char name[NAME_LEN];          // ≈ 2048 бит
    int is_dir;                   // 32 бита
} FileEntry;

typedef struct {
    char path[PATH_MAX];       // Текущий путь
    FileEntry files[MAX_FILES]; // Список файлов и папок
    int file_count;            // Количество элементов
    int selected;              // Индекс выбранного элемента
    int offset;                // Смещение для прокрутки
} Panel;

void load_directory(Panel* panel);
void delete_entry(Panel* panel);

#endif