#ifndef SEARCH_H
#define SEARCH_H
#define MAX_RESULTS 512
#include "limits.h"
#define MAX_DEPTH 10 
#define MAX_DIRS_SCANNED 5000
typedef struct {
    char results[MAX_RESULTS][PATH_MAX];  // ≈ 32 МБ
    int count;                            // 32 бита
    int selected;                         // 32 бита
    int active;                           // 32 бита
} SearchResult;

void search_files(const char* base_path, const char* query, SearchResult* result);
#endif