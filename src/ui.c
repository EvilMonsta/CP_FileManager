#include "ui.h"
#include <ncurses.h>
#include <string.h>
#include <stdbool.h>
#include <wchar.h>
#include <locale.h>
#include <stdlib.h>

#define PANEL_HEIGHT (LINES - 5)
#define COLOR_YES 1
#define COLOR_NO  2
#define COLOR_PANEL_BOX  3

void init_ui() {
    setlocale(LC_ALL, "");
    initscr(); raw(); noecho(); keypad(stdscr, TRUE); curs_set(0);
    start_color();
    init_pair(COLOR_YES, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_NO, COLOR_RED, COLOR_BLACK); 
    init_pair(COLOR_PANEL_BOX, COLOR_YELLOW, COLOR_BLACK);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
}

void close_ui() {
    endwin();
}

void draw_tabs(TabManager* manager) {
    int x = 0;
    for (int i = 0; i < manager->tab_count; i++) {
        if (i == manager->current_tab) attron(A_REVERSE | COLOR_PAIR(COLOR_PANEL_BOX));
        mvprintw(0, x, "[ Tab %d ]", i + 1);
        if (i == manager->current_tab) attroff(A_REVERSE | COLOR_PAIR(COLOR_PANEL_BOX));
        x += 10;
    }
    mvhline(1, 0, ACS_HLINE, COLS);
}

void draw_panel(Panel* panel, int x_offset, int width, int is_focused) {
    WINDOW* win = newwin(PANEL_HEIGHT, width, 2, x_offset);
    wattron(win, COLOR_PAIR(COLOR_PANEL_BOX));
    box(win, 0, 0);
    wattroff(win, COLOR_PAIR(COLOR_PANEL_BOX));

    int path_max_len = width - 4;
    char display_path[PATH_MAX + 1];
    strncpy(display_path, panel->path, path_max_len);
    display_path[path_max_len] = '\0';
    mvwprintw(win, 0, 2, "%s", display_path);

    int max_visible = PANEL_HEIGHT - 2;

    if (panel->selected < panel->offset) {
        panel->offset = panel->selected;
    } else if (panel->selected >= panel->offset + max_visible) {
        panel->offset = panel->selected - max_visible + 1;
    }

    for (int i = 0; i < max_visible; i++) {
        int idx = panel->offset + i;
        if (idx >= panel->file_count) break;

        const char* name = panel->files[idx].name;
        wchar_t wname[NAME_LEN];
        mbstowcs(wname, name, NAME_LEN);

        int name_max_width = width - 4;
        int printed_width = 0;
        int j = 0;
        wchar_t final[NAME_LEN];

        while (wname[j] != L'\0' && printed_width + wcwidth(wname[j]) <= name_max_width - 1) {
            final[j] = wname[j];
            printed_width += wcwidth(wname[j]);
            j++;
        }
        final[j] = L'\0';

        if (panel->files[idx].is_dir && printed_width + 1 < name_max_width) {
            final[j++] = L'/';
            final[j] = L'\0';
        }

        if (idx == panel->selected && is_focused) wattron(win, A_REVERSE);
        mvwaddwstr(win, i + 1, 1, final);
        if (idx == panel->selected && is_focused) wattroff(win, A_REVERSE);
    }

    wrefresh(win);
    delwin(win);
}

void draw_search_results(SearchResult* result) {
    int width = COLS - 4;
    int height = PANEL_HEIGHT;
    WINDOW* win = newwin(height, width, 2, 2);
    box(win, 0, 0);
    mvwprintw(win, 0, 0, " Search Results ");
    
    if (result->count == 0) {
        mvwprintw(win, 2, 2, "Nothing found. Press ESC to exit.");
        wrefresh(win);

        int ch;
        while ((ch = wgetch(win)) != 27) {} 
        delwin(win);
        return;
    }


    int max_visible = height - 2;
    int offset = 0;

    if (result->selected >= max_visible) {
        offset = result->selected - max_visible + 1;
    }
    if (result->limit_reached) {
        wattron(win, A_BOLD);
        mvwprintw(win, 0, width - 40, "[Directory scan limit reached]");
        wattroff(win, A_BOLD);
    }    

    for (int i = 0; i < max_visible; i++) {
        int idx = offset + i;
        if (idx >= result->count) break;

        if (idx == result->selected) wattron(win, A_REVERSE);
        mvwprintw(win, i + 1, 2, "%s", result->results[idx]);
        if (idx == result->selected) wattroff(win, A_REVERSE);
    }

    wrefresh(win);
    delwin(win);
}


void handle_input(int ch, Panel* panel) {
    if (ch == KEY_UP && panel->selected > 0) {
        panel->selected--;
    }
    else if (ch == KEY_DOWN && panel->selected < panel->file_count - 1) {
        panel->selected++;
    }
    else if (ch == 10) {
        FileEntry* entry = &panel->files[panel->selected];
        if (entry->is_dir) {
            if (strcmp(entry->name, ".") == 0) {
                load_directory(panel);
            }
            else if (strcmp(entry->name, "..") == 0) {
                char* last = strrchr(panel->path, '/');
                if (last && last != panel->path) *last = '\0';
                else strcpy(panel->path, "/");
                load_directory(panel);
            }
            else {
                if (strcmp(panel->path, "/") != 0) strcat(panel->path, "/");
                strcat(panel->path, entry->name);
                load_directory(panel);
            }
            panel->selected = 0;
            panel->offset = 0;
        }
    }
}

void draw_footer() {
    mvhline(LINES - 2, 0, ACS_HLINE, COLS);
    mvprintw(LINES - 1, 0, "F1:PrevTab  F2:NewTab  F3:CloseTab  F4:NextTab  F5:Rename  F6:Copy  F7:Search  F8:Delete  F9:Move  TAB:SwitchPanel  q:Quit");
}

bool confirm_deletion(const char* name) {
    int win_width = 50;
    int win_height = 7;
    int startx = (COLS - win_width) / 2;
    int starty = (LINES - win_height) / 2;

    WINDOW* win = newwin(win_height, win_width, starty, startx);
    keypad(win, TRUE);
    box(win, 0, 0);

    wattron(win, A_BOLD);
    mvwprintw(win, 1, 2, "Confirm deletion");
    wattroff(win, A_BOLD);

    char shortname[30];
    if (strlen(name) > 28) {
        snprintf(shortname, sizeof(shortname), "...%s", name + strlen(name) - 25);
    }
    else {
        strcpy(shortname, name);
    }

    mvwprintw(win, 3, 2, "Delete \"%s\"?", shortname);

    int yes_x = 4;
    int no_x = 20;
    int button_y = 5;

    wattron(win, COLOR_PAIR(COLOR_YES));
    mvwprintw(win, button_y, yes_x, "[Y] Yes");
    wattroff(win, COLOR_PAIR(COLOR_YES));

    wattron(win, COLOR_PAIR(COLOR_NO));
    mvwprintw(win, button_y, no_x, "[N] No");
    wattroff(win, COLOR_PAIR(COLOR_NO));

    wrefresh(win);

    int ch;
    bool result = false;
    MEVENT event;

    while (1) {
        ch = wgetch(win);
        if (ch == 'y' || ch == 'Y') {
            result = true;
            break;
        }
        else if (ch == 'n' || ch == 'N' || ch == 27) {
            result = false;
            break;
        }
        else if (ch == KEY_MOUSE) {
            if (getmouse(&event) == OK) {
                int mx = event.x;
                int my = event.y;

                if (my == starty + button_y) {
                    if (mx >= startx + yes_x && mx <= startx + yes_x + 6) {
                        result = true;
                        break;
                    }
                    if (mx >= startx + no_x && mx <= startx + no_x + 7) {
                        result = false;
                        break;
                    }
                }
            }
        }
    }

    delwin(win);
    return result;
}

bool prompt_search_query(char* out_query, size_t max_len) {
    int win_width = 60;
    int win_height = 9;
    int startx = (COLS - win_width) / 2;
    int starty = (LINES - win_height) / 2;

    WINDOW* win = newwin(win_height, win_width, starty, startx);
    keypad(win, TRUE);
    box(win, 0, 0);

    mvwprintw(win, 1, 2, "Search files");
    mvwprintw(win, 3, 2, "Enter query:");

    echo();
    curs_set(1);
    mvwgetnstr(win, 4, 2, out_query, max_len - 1);
    curs_set(0);
    noecho();

    int ok_x = 4;
    int cancel_x = 20;
    int button_y = 6;

    wattron(win, COLOR_PAIR(COLOR_YES));
    mvwprintw(win, button_y, ok_x, "[Enter] OK");
    wattroff(win, COLOR_PAIR(COLOR_YES));

    wattron(win, COLOR_PAIR(COLOR_NO));
    mvwprintw(win, button_y, cancel_x, "[ESC] Cancel");
    wattroff(win, COLOR_PAIR(COLOR_NO));

    wrefresh(win);

    int ch;
    bool result = false;
    MEVENT event;

    while (1) {
        ch = wgetch(win);
        if (ch == 10) {  
            result = true;
            break;
        } else if (ch == 27) {  
            result = false;
            break;
        } else if (ch == KEY_MOUSE) {
            if (getmouse(&event) == OK) {
                int mx = event.x;
                int my = event.y;
                if (my == starty + button_y) {
                    if (mx >= startx + ok_x && mx <= startx + ok_x + 11) {
                        result = true;
                        break;
                    }
                    if (mx >= startx + cancel_x && mx <= startx + cancel_x + 13) {
                        result = false;
                        break;
                    }
                }
            }
        }
    }

    delwin(win);
    clear();
    refresh();
    return result;
}

bool prompt_rename_file(const char* old_name, char* new_name, size_t max_len) {
    int win_width = 60;
    int win_height = 9;
    int startx = (COLS - win_width) / 2;
    int starty = (LINES - win_height) / 2;

    WINDOW* win = newwin(win_height, win_width, starty, startx);
    keypad(win, TRUE);
    box(win, 0, 0);

    wattron(win, A_BOLD);
    mvwprintw(win, 1, 2, "Rename File/Directory");
    wattroff(win, A_BOLD);

    mvwprintw(win, 3, 2, "Old name: %.*s", win_width - 15, old_name);
    mvwprintw(win, 4, 2, "New name:");
    wrefresh(win);

    echo();
    curs_set(1);
    mvwgetnstr(win, 4, 13, new_name, max_len - 1);
    noecho();
    curs_set(0);

    werase(win);
    wrefresh(win);
    delwin(win);
    touchwin(stdscr);
    refresh();

    return strlen(new_name) > 0;
}

bool confirm_rename(const char* old_name, const char* new_name) {
    int win_width = 60;
    int win_height = 9;
    int startx = (COLS - win_width) / 2;
    int starty = (LINES - win_height) / 2;

    WINDOW* win = newwin(win_height, win_width, starty, startx);
    box(win, 0, 0);
    keypad(win, TRUE);

    mvwprintw(win, 1, 2, "Confirm Rename");
    mvwprintw(win, 3, 2, "From: \"%.*s\"", win_width - 10, old_name);
    mvwprintw(win, 4, 2, "To:   \"%.*s\"", win_width - 10, new_name);

    int yes_x = 6;
    int no_x = 24;
    int button_y = 6;

    wattron(win, COLOR_PAIR(COLOR_YES));
    mvwprintw(win, button_y, yes_x, "[Y] Yes");
    wattroff(win, COLOR_PAIR(COLOR_YES));

    wattron(win, COLOR_PAIR(COLOR_NO));
    mvwprintw(win, button_y, no_x, "[N] No");
    wattroff(win, COLOR_PAIR(COLOR_NO));

    wrefresh(win);

    int ch;
    bool result = false;
    MEVENT event;

    while (1) {
        ch = wgetch(win);
        if (ch == 'y' || ch == 'Y') {
            result = true;
            break;
        } else if (ch == 'n' || ch == 'N' || ch == 27) {
            result = false;
            break;
        } else if (ch == KEY_MOUSE) {
            if (getmouse(&event) == OK) {
                int mx = event.x;
                int my = event.y;
                if (my == starty + button_y) {
                    if (mx >= startx + yes_x && mx <= startx + yes_x + 7) {
                        result = true;
                        break;
                    }
                    if (mx >= startx + no_x && mx <= startx + no_x + 6) {
                        result = false;
                        break;
                    }
                }
            }
        }
    }

    werase(win);
    wrefresh(win);
    delwin(win);
    touchwin(stdscr);
    refresh();
    return result;
}

bool confirm_copy(const char* src, const char* dest) {
    int win_width = 60;
    int win_height = 9;
    int startx = (COLS - win_width) / 2;
    int starty = (LINES - win_height) / 2;

    WINDOW* win = newwin(win_height, win_width, starty, startx);
    box(win, 0, 0);
    keypad(win, TRUE);

    mvwprintw(win, 1, 2, "Confirm Copy");
    mvwprintw(win, 3, 2, "From: \"%.*s\"", win_width - 10, src);
    mvwprintw(win, 4, 2, "To:   \"%.*s\"", win_width - 10, dest);

    int yes_x = 6;
    int no_x = 24;
    int button_y = 6;

    wattron(win, COLOR_PAIR(COLOR_YES));
    mvwprintw(win, button_y, yes_x, "[Y] Yes");
    wattroff(win, COLOR_PAIR(COLOR_YES));

    wattron(win, COLOR_PAIR(COLOR_NO));
    mvwprintw(win, button_y, no_x, "[N] No");
    wattroff(win, COLOR_PAIR(COLOR_NO));

    wrefresh(win);

    int ch;
    bool result = false;
    MEVENT event;

    while (1) {
        ch = wgetch(win);
        if (ch == 'y' || ch == 'Y') { result = true; break; }
        else if (ch == 'n' || ch == 'N' || ch == 27) { result = false; break; }
        else if (ch == KEY_MOUSE) {
            if (getmouse(&event) == OK) {
                int mx = event.x;
                int my = event.y;
                if (my == starty + button_y) {
                    if (mx >= startx + yes_x && mx <= startx + yes_x + 7) {
                        result = true;
                        break;
                    }
                    if (mx >= startx + no_x && mx <= startx + no_x + 6) {
                        result = false;
                        break;
                    }
                }
            }
        }
    }

    werase(win); wrefresh(win); delwin(win);
    touchwin(stdscr); refresh();
    return result;
}

bool confirm_overwrite(const char* dest_name) {
    int win_width = 50;
    int win_height = 7;
    int startx = (COLS - win_width) / 2;
    int starty = (LINES - win_height) / 2;

    WINDOW* win = newwin(win_height, win_width, starty, startx);
    box(win, 0, 0);
    keypad(win, TRUE);

    mvwprintw(win, 1, 2, "File \"%.*s\" already exists.", win_width - 10, dest_name);
    mvwprintw(win, 3, 2, "Overwrite it?");

    int yes_x = 6;
    int no_x = 24;
    int button_y = 5;

    wattron(win, COLOR_PAIR(COLOR_YES));
    mvwprintw(win, button_y, yes_x, "[Y] Yes");
    wattroff(win, COLOR_PAIR(COLOR_YES));

    wattron(win, COLOR_PAIR(COLOR_NO));
    mvwprintw(win, button_y, no_x, "[N] No");
    wattroff(win, COLOR_PAIR(COLOR_NO));

    wrefresh(win);

    int ch;
    bool result = false;
    MEVENT event;

    while (1) {
        ch = wgetch(win);
        if (ch == 'y' || ch == 'Y') { result = true; break; }
        else if (ch == 'n' || ch == 'N' || ch == 27) { result = false; break; }
        else if (ch == KEY_MOUSE) {
            if (getmouse(&event) == OK) {
                int mx = event.x;
                int my = event.y;
                if (my == starty + button_y) {
                    if (mx >= startx + yes_x && mx <= startx + yes_x + 7) {
                        result = true;
                        break;
                    }
                    if (mx >= startx + no_x && mx <= startx + no_x + 6) {
                        result = false;
                        break;
                    }
                }
            }
        }
    }

    werase(win); wrefresh(win); delwin(win);
    touchwin(stdscr); refresh();
    return result;
}
