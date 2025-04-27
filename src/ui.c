#include "ui.h"
#include <ncurses.h>
#include <string.h>

#define PANEL_HEIGHT (LINES - 5)

void init_ui() {
    initscr(); raw(); noecho(); keypad(stdscr, TRUE); curs_set(0);
}

void close_ui() {
    endwin();
}

void draw_tabs(TabManager* manager) {
    int x = 0;
    for (int i = 0; i < manager->tab_count; i++) {
        if (i == manager->current_tab) attron(A_REVERSE);
        mvprintw(0, x, "[ Tab %d ]", i + 1);
        if (i == manager->current_tab) attroff(A_REVERSE);
        x += 10;
    }
    mvhline(1, 0, ACS_HLINE, COLS);
}

void draw_panel(Panel* panel, int x_offset, int width, int is_focused) {
    WINDOW* win = newwin(PANEL_HEIGHT, width, 2, x_offset);
    box(win, 0, 0);

    mvwprintw(win, 0, 2, "%.30s", panel->path);

    int max_visible = PANEL_HEIGHT - 2;

    if (panel->selected < panel->offset) {
        panel->offset = panel->selected;
    } else if (panel->selected >= panel->offset + max_visible) {
        panel->offset = panel->selected - max_visible + 1;
    }

    for (int i = 0; i < max_visible; i++) {
        int idx = panel->offset + i;
        if (idx >= panel->file_count) break;

        if (idx == panel->selected && is_focused) wattron(win, A_REVERSE);
        mvwprintw(win, i + 1, 1, "%-*s%s", width - 3, panel->files[idx].name, panel->files[idx].is_dir ? "/" : "");
        if (idx == panel->selected && is_focused) wattroff(win, A_REVERSE);
    }

    wrefresh(win);
    delwin(win);
}


void draw_search_results(SearchResult* result) {
    mvprintw(2, 2, "Search result:");

    if (result->count == 0) {
        mvprintw(4, 4, "Nothing found. Press ESC to exit.");
        return;
    }

    int max_visible = PANEL_HEIGHT;
    int offset = 0;

    if (result->selected >= max_visible) {
        offset = result->selected - max_visible + 1;
    }

    for (int i = 0; i < max_visible; i++) {
        int idx = offset + i;
        if (idx >= result->count) break;

        if (idx == result->selected) attron(A_REVERSE);
        mvprintw(i + 3, 4, "%s", result->results[idx]);
        if (idx == result->selected) attroff(A_REVERSE);
    }
}

void handle_input(int ch, Panel* panel) {
    if (ch == KEY_UP && panel->selected > 0) {
        panel->selected--;
    } else if (ch == KEY_DOWN && panel->selected < panel->file_count - 1) {
        panel->selected++;
    } else if (ch == 10) {
        FileEntry* entry = &panel->files[panel->selected];
        if (entry->is_dir) {
            if (strcmp(entry->name, ".") == 0) {
                load_directory(panel);
            } else if (strcmp(entry->name, "..") == 0) {
                char* last = strrchr(panel->path, '/');
                if (last && last != panel->path) *last = '\0';
                else strcpy(panel->path, "/");
                load_directory(panel);
            } else {
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
    mvprintw(LINES - 1, 0, "F1:PrevTab  F2:NewTab  F3:CloseTab  F4:NextTab  F7:Search  F8:Delete  TAB:SwitchPanel  q:Quit");
}
bool confirm_deletion(const char* name) {
    int mid_y = LINES / 2;
    int mid_x = COLS / 2 - 20;

    attron(A_BOLD);
    mvprintw(mid_y, mid_x, "Delete \"%s\"? [y/n]", name);
    attroff(A_BOLD);
    refresh();

    int ch;
    while (1) {
        ch = getch();
        if (ch == 'y' || ch == 'Y') return true;
        if (ch == 'n' || ch == 'N' || ch == 27) return false;
    }
}