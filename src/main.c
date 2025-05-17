#include "ui.h"
#include "copy.h"
#include "fileops.h"
#include "tabmanager.h"
#include "search.h"
#include <ncurses.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h> //access()

void render(TabManager* manager, SearchResult* search_result, int focused) {
    draw_tabs(manager);

    if (search_result->active) {
        draw_search_results(search_result);
    }
    else {
        Tab* tab = &manager->tabs[manager->current_tab];
        int panel_width = COLS / 2;

        draw_panel(&tab->left, 0, panel_width, focused == 0);
        draw_panel(&tab->right, panel_width, panel_width, focused == 1);

        mvprintw(1, COLS - 20, "Tab: %d/%d", manager->current_tab + 1, manager->tab_count);
    }

    draw_footer();
}

int main() {
    TabManager manager;
    int focused = 0;
    SearchResult search_result = { 0 };

    init_tabs(&manager, ".");
    init_ui();
    refresh();
    render(&manager, &search_result, focused);
    int ch;
    MEVENT event;

    while ((ch = getch()) != 'q') {
        Tab* tab = &manager.tabs[manager.current_tab];

        if (ch == KEY_MOUSE) {
            if (getmouse(&event) == OK) {
                int mx = event.x;
                int my = event.y;
                int y = LINES - 1;

                if (my == y) {
                    if (mx >= 0 && mx < 10) ch = KEY_F(1);           // F1:PrevTab
                    else if (mx >= 12 && mx < 21) ch = KEY_F(2);     // F2:NewTab
                    else if (mx >= 23 && mx < 34) ch = KEY_F(3);     // F3:CloseTab
                    else if (mx >= 36 && mx < 46) ch = KEY_F(4);      // F4:NextTab
                    else if (mx >= 48  && mx < 57) ch = KEY_F(5);      // F5:Rename  
                    else if (mx >= 59 && mx < 68) ch = KEY_F(7);     // F7:Search
                    else if (mx >= 70 && mx < 79) ch = KEY_F(8);     // F8:Delete
                    else if (mx >= 81 && mx < 96) ch = '	';         // TAB:SwitchPanel
                    else if (mx >= 98 && mx < 104) ch = 'q';         // q:Quit
                }
            }
        }

        if (search_result.active) {
            if (ch == KEY_UP && search_result.selected > 0) {
                search_result.selected--;
            }
            else if (ch == KEY_DOWN && search_result.selected < search_result.count - 1) {
                search_result.selected++;
            }
            else if (ch == 27) {
                search_result.active = 0;
                render(&manager, &search_result, focused);
            }
            else if (ch == 10) {
                const char* selected_path = search_result.results[search_result.selected];
                struct stat st;
                if (stat(selected_path, &st) == 0) {
                    Panel* panel = (focused == 0) ? &tab->left : &tab->right;
                    if (S_ISDIR(st.st_mode)) {
                        strncpy(panel->path, selected_path, PATH_MAX);
                    }
                    else {
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
        }
        else {
            if (ch == '\t') {
                focused = 1 - focused;
            }
            else if (ch == KEY_F(1)) {
                switch_tab(&manager, -1);
            }
            else if (ch == KEY_F(2)) {
                add_tab(&manager, ".");
            }
            else if (ch == KEY_F(3)) {
                close_tab(&manager, manager.current_tab);
            }
            else if (ch == KEY_F(4)) {
                switch_tab(&manager, 1);
            }
            else if (ch == KEY_F(5)) {
                Panel* panel = (focused == 0) ? &tab->left : &tab->right;
                if (panel->file_count == 0) continue;
            
                FileEntry* entry = &panel->files[panel->selected];
                if (!strcmp(entry->name, ".") || !strcmp(entry->name, "..")) continue;
            
                char new_name[NAME_LEN];
                if (!prompt_rename_file(entry->name, new_name, sizeof(new_name))) continue;
            
                char old_path[PATH_MAX];
                char new_path[PATH_MAX];
            
                strncpy(old_path, panel->path, sizeof(old_path) - 1);
                old_path[sizeof(old_path) - 1] = '\0';
                if (old_path[strlen(old_path) - 1] != '/')
                    strncat(old_path, "/", sizeof(old_path) - strlen(old_path) - 1);
                strncat(old_path, entry->name, sizeof(old_path) - strlen(old_path) - 1);

                strncpy(new_path, panel->path, sizeof(new_path) - 1);
                new_path[sizeof(new_path) - 1] = '\0';
                if (new_path[strlen(new_path) - 1] != '/')
                    strncat(new_path, "/", sizeof(new_path) - strlen(new_path) - 1);
                strncat(new_path, new_name, sizeof(new_path) - strlen(new_path) - 1);
            
                if (!confirm_rename(entry->name, new_name)) {
                    render(&manager, &search_result, focused);
                    continue;
                }

                if (access(new_path, F_OK) == 0) {
                    mvprintw(1, 2, "A file with that name already exists.");
                    refresh();
                    getch();
                    refresh();
                    render(&manager, &search_result, focused);
                    continue;
                }
                
                if (rename(old_path, new_path) != 0) {
                    mvprintw(1, 2, "Rename failed.");
                    refresh();
                    getch();
                    render(&manager, &search_result, focused);
                } else {
                    load_directory(panel);
                }
            }       
            else if (ch == KEY_F(6)) {
                Panel* src_panel = (focused == 0) ? &tab->left : &tab->right;
                Panel* dst_panel = (focused == 0) ? &tab->right : &tab->left;
                if (src_panel->file_count == 0) continue;
            
                FileEntry* entry = &src_panel->files[src_panel->selected];
                if (!strcmp(entry->name, ".") || !strcmp(entry->name, "..")) continue;
            
                char src_path[PATH_MAX], dest_path[PATH_MAX];

                strncpy(src_path, src_panel->path, sizeof(src_path) - 1);
                src_path[sizeof(src_path) - 1] = '\0';
                if (src_path[strlen(src_path) - 1] != '/')
                    strncat(src_path, "/", sizeof(src_path) - strlen(src_path) - 1);
                strncat(src_path, entry->name, sizeof(src_path) - strlen(src_path) - 1);

                strncpy(dest_path, dst_panel->path, sizeof(dest_path) - 1);
                dest_path[sizeof(dest_path) - 1] = '\0';
                if (dest_path[strlen(dest_path) - 1] != '/')
                    strncat(dest_path, "/", sizeof(dest_path) - strlen(dest_path) - 1);
                strncat(dest_path, entry->name, sizeof(dest_path) - strlen(dest_path) - 1);

                if (access(dest_path, F_OK) == 0) {
                    mvprintw(1, 2, "Copy failed: target already exists.");
                    getch();
                    continue;
                }
            
                if (!confirm_copy(entry->name, dest_path)) {
                    refresh();
                    render(&manager, &search_result, focused);
                    continue;
                }
                if (!copy_entry(src_path, dest_path)) {
                    mvprintw(1, 2, "Copy failed.");
                    getch();
                } else {
                    load_directory(dst_panel);
                }
            }     
            else if (ch == KEY_F(7)) {
                char query[256];
                if (!prompt_search_query(query, sizeof(query))) {
                    search_result.active = 0;
                    render(&manager, &search_result, focused);
                    continue; 
                }
                

                const char* base = (focused == 0) ? tab->left.path : tab->right.path;
                search_files(base, query, &search_result);

                if (search_result.count == 0) {
                    draw_search_results(&search_result);
                    search_result.active = 0;
                    render(&manager, &search_result, focused);
                }
            }
            else if (ch == KEY_F(8)) {
                Panel* panel = (focused == 0) ? &tab->left : &tab->right;
                if (panel->file_count == 0) continue;

                FileEntry* entry = &panel->files[panel->selected];
                if (!strcmp(entry->name, ".") || !strcmp(entry->name, "..")) continue;

                if (confirm_deletion(entry->name)) {
                    delete_entry(panel);
                }
            }
            else if (focused == 0) {
                handle_input(ch, &tab->left);
            }
            else {
                handle_input(ch, &tab->right);
            }
        }

        render(&manager, &search_result, focused);
    }

    close_ui();
    return 0;
}
