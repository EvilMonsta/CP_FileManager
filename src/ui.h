#ifndef UI_H
#define UI_H

#include "fileops.h"
#include "search.h"
#include <stdbool.h>
#include "tabmanager.h"

void init_ui();
void close_ui();
void draw_panel(Panel* panel, int x_offset, int width, int is_focused);
void draw_search_results(SearchResult* result);
void handle_input(int ch, Panel* panel);
void draw_footer();
bool confirm_deletion(const char* name);
void draw_tabs(TabManager* manager);
#endif
