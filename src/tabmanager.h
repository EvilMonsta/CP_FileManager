#ifndef TABMANAGER_H
#define TABMANAGER_H
#include "fileops.h"
#define MAX_TABS 10

typedef struct {
    Panel left;  
    Panel right;  
} Tab;

typedef struct {
    Tab tabs[MAX_TABS];   
    int tab_count;       
    int current_tab;    
} TabManager;

void init_tabs(TabManager* manager, const char* start_path);
void switch_tab(TabManager* manager, int direction);
void add_tab(TabManager* manager, const char* start_path);
void close_tab(TabManager* manager, int index);
#endif