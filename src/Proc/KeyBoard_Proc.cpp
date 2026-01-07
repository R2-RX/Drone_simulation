#include <locale.h>
#include <ncurses.h>
#include <iostream>
#include <stdio.h>
#include "Ncurses_Win.h"
#include "Keyboard.h"
#include "Pipe.h"
#include <cstring>
#include "BlackBoard.h"
#include "Logger.h"

Logger logger(SYSTEM_WIDE_LOG);
static volatile bool shutdownFlag = false;

void draw_tail_output(WINDOW* win);
void print_center(WINDOW* win, int row, const char* str);
void draw_keys(WINDOW* win);
void handle_sigterm(int signum) {shutdownFlag = true;}

int main() {
    signal(SIGTERM, handle_sigterm); // from watchdog
    signal(SIGINT, handle_sigterm); // Ctrl+C

    setlocale(LC_ALL, "");

    Ncurses_Win app;
    Keyboard Key_Command;
    BlackBoard blackboard;

    WINDOW* win = app.getWindow();
    int ch;

    draw_keys(win);

    blackboard.setProcessPid(WatchDogProcName::Keyboard_Proc, getppid());

    while (true) {

        //------ send a msg to watch dog ----- 
        Pipe<char> keyboard_pipe_wd(KEYBOARD_PIPE_WD);
        keyboard_pipe_wd.send_data('K');
        //-----------------------------------

        ch = getch();         // Non-blocking

        if (ch == 27){
            keyboard_pipe_wd.send_data('Q');  // send quit signal
            break;  // ESC to exit
        } 
        if (shutdownFlag) {
            logger.log("Received SIGTERM, shutting down...", getpid(),Logger::LogLevel::WARNING);
            break; 
        }

        if (ch != ERR) {
            Key_Command.update(ch);
        }

        draw_tail_output(win);  // Always update tail
        draw_keys(win);         // Keep static keys on screen

        // Handle resize
        if (ch == KEY_RESIZE) {
            app.resize();
            draw_keys(win);
        }

        napms(10); // Small delay to reduce CPU usage
    }

    return 0;

}

void draw_tail_output(WINDOW* win) {

    for (int i = 10; i < getmaxy(win) - 1; ++i) {
        mvwprintw(win, i, 0, "%*s", getmaxx(win) - 1, " ");
    }

    std::string str = std::string("tail -n 20 ") + LIVE_MONITORING;
    FILE* pipe = popen(str.c_str(), "r");
    if (!pipe) return;

    char buffer[256];
    int row = 10;

    while (fgets(buffer, sizeof(buffer), pipe) && row < getmaxy(win) - 1) {
        wattron(win, COLOR_PAIR(6) | A_BOLD);
        mvwprintw(win, row++, 1, "%s", buffer);
        wattroff(win, COLOR_PAIR(6) | A_BOLD);
    }

    pclose(pipe);
    wrefresh(win);

}

void print_center(WINDOW* win, int row, const char* str) {
    int width = getmaxx(win);
    int start_col = (width - strlen(str)) / 2;
    mvwprintw(win, row, start_col, "%s", str);
}

void draw_keys(WINDOW* win) {
    wattron(win, COLOR_PAIR(3) | A_BOLD);  
    print_center(win, 1, "Use the following keys to move:");
    print_center(win, 3, "Q W E");
    print_center(win, 4, "A S D");
    print_center(win, 5, "Z X C");
    print_center(win, 7, "Arrow Keys:");
    print_center(win, 9, "   ↑ ");
    print_center(win, 10, "    ←   →");
    print_center(win, 11, "   ↓ ");
    wattroff(win, COLOR_PAIR(3) | A_BOLD);
    wrefresh(win);
}
