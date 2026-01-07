// Keyboard.cpp
#include "Keyboard.h"
#include "Ncurses_Win.h"
#include <stdexcept>
#include "config.h"
#include "Pipe.h"

Keyboard::Keyboard()
{
    initscr();
    cbreak();
    noecho();
    curs_set(0);              // Hide cursor
    nodelay(stdscr, TRUE);   // wait for key press = false
    keypad(stdscr, TRUE);     // Enable special keys (including KEY_RESIZE)
    last_Force_command_ = {0.0, 0.0};
}

Point Keyboard::readInput(int ch)
{
    // Debug ncurses
    //mvprintw(1, 12, "Key code: %d   ", ch);
    //refresh();
    
    if (ch == ERR)
         return last_Force_command_;      // keep previous direction

    switch (ch) {
        case 'w': case KEY_UP:    return { 0.0, -1.0 - DRONE_ASSIST_FORCE };
        case 's': case KEY_DOWN:  return { 0.0,  1.0 + DRONE_ASSIST_FORCE};
        case 'a': case KEY_LEFT:  return {-1.0 - DRONE_ASSIST_FORCE,  0.0 };
        case 'd': case KEY_RIGHT: return { 1.0 + DRONE_ASSIST_FORCE,  0.0 };

        // diagonal movement
        case 'q': return {-1.0 - DRONE_ASSIST_FORCE, -1.0 - DRONE_ASSIST_FORCE};
        case 'e': return { 1.0 + DRONE_ASSIST_FORCE, -1.0 - DRONE_ASSIST_FORCE};
        case 'z': return {-1.0 - DRONE_ASSIST_FORCE,  1.0 + DRONE_ASSIST_FORCE};
        case 'c': return { 1.0 + DRONE_ASSIST_FORCE,  1.0 + DRONE_ASSIST_FORCE};

        case 'x': return { 0.0,  0.0 }; // no thrust, forces set to zero
    }

    return last_Force_command_;   // unknown key
}

void Keyboard::update(int ch)
{
    last_Force_command_ = readInput(ch);

    // Show result using ncurses
    // mvprintw(1, 0, "Command: %.1f , %.1f   ",
    //          last_Force_command_.x,
    //          last_Force_command_.y);
    Pipe<Point> Pipe_(KEYBOARD_Data_PIPE);
    Pipe_.send_data(last_Force_command_);
    //refresh();
}

// --- Getter ---
Point Keyboard::getCommand() const
{
    return last_Force_command_;
}
