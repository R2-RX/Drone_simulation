#ifndef NCURSES_WIN_H
#define NCURSES_WIN_H

#include <locale.h>
#include <ncurses.h>
#include <stdexcept>
#include <utility>
#include "Space2DLogic.h"
#include "BlackBoard.h"

#define COLOR_ATTR(pair, attr) (COLOR_PAIR(pair) | (attr))
class Ncurses_Win {
protected:
    int H, W;      // Terminal height/width
    int wh, ww;    // Window height/width
    int wy, wx;    // Top-left position of centered window
    WINDOW* win;   // The main ncurses window

    void updateDimensions();     // Recalculate window/terminal sizes
    void redrawFrame();          // Draw border and title

public:
    Ncurses_Win();               // Constructor initializes ncurses
    ~Ncurses_Win();              // Destructor cleans up safely

    void resize();               // Handle terminal resize
    std::pair<int,int> getcurrentsize() const;

    void drawAll(Space2D& space, WINDOW* win, BlackBoard& BB, double scale);
    void setup_colors();
    void print_centered(WINDOW* win, int row, const char* str, attr_t attr = COLOR_PAIR(2));
    void destroy();

    WINDOW* getWindow() const { return win; }
};

#endif
