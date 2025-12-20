#include "Ncurses_Win.h"

Ncurses_Win::Ncurses_Win()
    : H(0), W(0), wh(0), ww(0), wy(0), wx(0), win(nullptr)
{
    setlocale(LC_ALL, "");
    setenv("TERM", "xterm-256color", 1);
    initscr();
    start_color(); // initialize color functionality
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    updateDimensions();
    win = newwin(wh, ww, wy, wx);
    if (!win) {
        endwin();
        throw std::runtime_error("Failed to create ncurses window");
    }

    setup_colors(); // Setup the colors for each object
    redrawFrame();
}

Ncurses_Win::~Ncurses_Win() {
    if (win) delwin(win);
    endwin();
}

void Ncurses_Win::updateDimensions() {
    getmaxyx(stdscr, H, W);

    wh = (H > 10) ? H - 5 : H;
    ww = (W > 20) ? W - 10 : W;

    if (wh < 5)  wh = 5;
    if (ww < 10) ww = 10;

    wy = (H - wh) / 2;
    wx = (W - ww) / 2;
}

void Ncurses_Win::redrawFrame() {
    erase();          // correct way to clear stdscr
    werase(win);      // clear window

    int top    = wy - 1;
    int left   = wx - 1;
    int bottom = wy + wh;
    int right  = wx + ww;

    // Horizontal borders
    mvhline(top,    left,  ACS_HLINE, ww + 2);
    mvhline(bottom, left,  ACS_HLINE, ww + 2);

    // Vertical borders
    mvvline(top, left,  ACS_VLINE, wh + 2);
    mvvline(top, right, ACS_VLINE, wh + 2);

    // Corners
    mvaddch(top,    left,  ACS_ULCORNER);
    mvaddch(top,    right, ACS_URCORNER);
    mvaddch(bottom, left,  ACS_LLCORNER);
    mvaddch(bottom, right, ACS_LRCORNER);

    mvprintw(0, 0, "Press 'ESC' to exit | Terminal: %dx%d | Window: %dx%d",
             W, H, ww, wh);

    refresh();
    wrefresh(win);
}

void Ncurses_Win::resize() {
    updateDimensions();

    wresize(win, wh, ww);
    mvwin(win, wy, wx);

    wclear(win);
    redrawFrame();
}

std::pair<int,int> Ncurses_Win::getcurrentsize() const {
    int rows, cols;
    getmaxyx(win, rows, cols);
    return {cols, rows};   // (width, height)
}

void Ncurses_Win::drawAll(Space2D&, WINDOW* win, BlackBoard& BB, double scale) {
    werase(win);

    int h, w;
    getmaxyx(win, h, w);

    for (ItemLogic* logic : BB.getAllLogicObjects()) {
        int sx = static_cast<int>(logic->scaledX(scale));
        int sy = static_cast<int>(logic->scaledY(scale));

        // Prevent out-of-bounds crashes
        if (sy >= 0 && sy < h && sx >= 0 && sx < w) {
            switch (logic->get_data_ptr()->type) {
                case ItemData::ItemType::Target:
                    // Apply red color for Target
                    wattron(win, COLOR_PAIR(3));
                    mvwaddch(win, sy, sx, logic->symbol() | A_BOLD);
                    wattroff(win, COLOR_PAIR(3));
                    break;

                case ItemData::ItemType::Obstacle:
                    // Apply blue color for Obstacle
                    wattron(win, COLOR_PAIR(4));
                    mvwaddch(win, sy, sx, logic->symbol());
                    wattroff(win, COLOR_PAIR(4));
                    break;

                case ItemData::ItemType::Drone:
                    // Apply green color for Drone
                    wattron(win, COLOR_PAIR(2));
                    mvwaddch(win, sy, sx, logic->symbol() | A_BOLD);
                    wattroff(win, COLOR_PAIR(2));
                    break;

            }
        }
    }

    wrefresh(win);
}

void Ncurses_Win::setup_colors() {
    // Initialize color pairs
    init_pair(1, COLOR_RED,     COLOR_BLACK);
    init_pair(2, COLOR_GREEN,   COLOR_BLACK);
    init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
    init_pair(4, COLOR_BLUE,    COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_CYAN,    COLOR_BLACK);
    init_pair(7, COLOR_WHITE,   COLOR_BLACK);

}
