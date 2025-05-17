#ifndef UI_H
#define UI_H

#include "fileops.h"
#include "search.h"
#include <stdbool.h>
#include "tabmanager.h"
#include <stdio.h>
void init_ui();
void close_ui();
void draw_panel(Panel* panel, int x_offset, int width, int is_focused);
void draw_search_results(SearchResult* result);
void handle_input(int ch, Panel* panel);
void draw_footer();
void draw_tabs(TabManager* manager);
bool prompt_search_query(char* out_query, size_t max_len);
bool prompt_rename_file(const char* old_name, char* new_name, size_t max_len);

bool confirm_deletion(const char* name);
bool confirm_rename(const char* old_name, const char* new_name);
bool confirm_copy(const char* src, const char* dest);

#endif
