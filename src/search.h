#ifndef SEARCH_H
#define SEARCH_H
#define MAX_RESULTS 512
#include "limits.h"
#define MAX_DEPTH 10 
#define MAX_DIRS_SCANNED 50000
typedef struct {
    char results[MAX_RESULTS][PATH_MAX];  
    int count;                          
    int selected;                        
    int active;            
    int limit_reached;             
} SearchResult;

void search_files(const char* base_path, const char* query, SearchResult* result);
#endif