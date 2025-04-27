#include "ui.h"
#include "fileops.h"
#include "tabmanager.h"
#include "search.h"
#include <ncurses.h>
#include <string.h>
#include <sys/stat.h>

void render(TabManager* manager, SearchResult* search_result, int focused) {
    clear();

    draw_tabs(manager);

    if (search_result->active) {
        draw_search_results(search_result);
    } else {
        Tab* tab = &manager->tabs[manager->current_tab];
        int panel_width = COLS / 2;

        draw_panel(&tab->left, 0, panel_width, focused == 0);
        draw_panel(&tab->right, panel_width, panel_width, focused == 1);

        mvprintw(1, COLS - 20, "Tab: %d/%d", manager->current_tab + 1, manager->tab_count);
    }

    draw_footer();

    refresh();
}
int main() {
    TabManager manager;
    int focused = 0;
    SearchResult search_result = {0};

    init_tabs(&manager, ".");
    init_ui();
    render(&manager, &search_result, focused);
    int ch;
    while ((ch = getch()) != 'q') {
        Tab* tab = &manager.tabs[manager.current_tab];

        if (search_result.active) {
            if (ch == KEY_UP && search_result.selected > 0) {
                search_result.selected--;
            } else if (ch == KEY_DOWN && search_result.selected < search_result.count - 1) {
                search_result.selected++;
            } else if (ch == 27) {
                search_result.active = 0;
            } else if (ch == 10) {
                const char* selected_path = search_result.results[search_result.selected];
                struct stat st;
                if (stat(selected_path, &st) == 0) {
                    Panel* panel = (focused == 0) ? &tab->left : &tab->right;
                    if (S_ISDIR(st.st_mode)) {
                        strncpy(panel->path, selected_path, PATH_MAX);
                    } else {
                        char temp[PATH_MAX];
                        strncpy(temp, selected_path, PATH_MAX);
                        char* last_slash = strrchr(temp, '/');
                        if (last_slash) {
                            *last_slash = '\0';
                            strncpy(panel->path, temp, PATH_MAX);
                        }
                    }
                    panel->selected = 0;
                    load_directory(panel);
                }
                search_result.active = 0;
            }
        } else {
            if (ch == '\t') {
                focused = 1 - focused;
            } else if (ch == KEY_F(1)) {
                switch_tab(&manager, -1);
            } else if (ch == KEY_F(2)) {
                add_tab(&manager, ".");
            } else if (ch == KEY_F(3)) {
                close_tab(&manager, manager.current_tab);
            } else if (ch == KEY_F(4)) {
                switch_tab(&manager, 1);
            } else if (ch == KEY_F(7)) {
                echo();
            char query[256];
            mvprintw(0, 0, "Search: ");
            getnstr(query, 255);
            noecho();

            const char* base = (focused == 0) ? tab->left.path : tab->right.path;
            search_files(base, query, &search_result);

            if (search_result.count == 0) {
                clear();
                draw_search_results(&search_result);
                refresh();
                getch();  
                search_result.active = 0;
            }
            } else if (ch == KEY_F(8)) {
                Panel* panel = (focused == 0) ? &tab->left : &tab->right;
                if (panel->file_count == 0) continue;

                FileEntry* entry = &panel->files[panel->selected];
                if (!strcmp(entry->name, ".") || !strcmp(entry->name, "..")) continue;

                if (confirm_deletion(entry->name)) {
                    delete_entry(panel);
                }
            } else if (focused == 0) {
                handle_input(ch, &tab->left);
            } else {
                handle_input(ch, &tab->right);
            }
        }
        render(&manager, &search_result, focused);

        if (search_result.active) {
            draw_search_results(&search_result);
        } else {
            int panel_width = COLS / 2;
            draw_panel(&tab->left, 0, panel_width, focused == 0);
            draw_panel(&tab->right, panel_width, panel_width, focused == 1);
        }

    }

    close_ui();
    return 0;
}