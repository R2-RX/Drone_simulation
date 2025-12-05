#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <utility> // For std::pair
#include "Ncurses_Win.h"
#include "Pipe.h"
#include "config.h"

class Keyboard : Ncurses_Win {
public:
    Keyboard();
    ~Keyboard() = default;
    Keyboard(const Keyboard&) = delete;
    Keyboard& operator=(const Keyboard&) = delete;

    void update(int ch);                                      // should be called every loop
    Pair_ getCommand() const;        // std::pair<double, double> 

private:
    Pair_ readInput(int ch);              

    Pair_ last_Force_command_ {0.0, 0.0};
};

#endif 