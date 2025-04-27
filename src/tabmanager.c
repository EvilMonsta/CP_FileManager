#include "tabmanager.h"
#include <string.h>

void init_tabs(TabManager* manager, const char* start_path) {
    manager->tab_count = 1;
    manager->current_tab = 0;
    strcpy(manager->tabs[0].left.path, start_path);
    strcpy(manager->tabs[0].right.path, start_path);
    load_directory(&manager->tabs[0].left);
    load_directory(&manager->tabs[0].right);
}

void switch_tab(TabManager* manager, int direction) {
    if (manager->tab_count == 0) return;
    manager->current_tab = (manager->current_tab + direction + manager->tab_count) % manager->tab_count;
}

void add_tab(TabManager* manager, const char* start_path) {
    if (manager->tab_count >= MAX_TABS) return;
    int i = manager->tab_count;
    strcpy(manager->tabs[i].left.path, start_path);
    strcpy(manager->tabs[i].right.path, start_path);
    load_directory(&manager->tabs[i].left);
    load_directory(&manager->tabs[i].right);
    manager->tab_count++;
    manager->current_tab = i;
}

void close_tab(TabManager* manager, int index) {
    if (manager->tab_count <= 1) return;
    for (int i = index; i < manager->tab_count - 1; i++) {
        manager->tabs[i] = manager->tabs[i + 1];
    }
    manager->tab_count--;
    if (manager->current_tab >= manager->tab_count) {
        manager->current_tab = manager->tab_count - 1;
    }
}