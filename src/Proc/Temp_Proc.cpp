#include <ncurses.h>
#include <chrono>
#include <thread>

//typedef  std::chrono::steady_clock clock_t;

const double FIXED_DELTA = 1.0 / 60.0;

// ----------- GAME STATE (current + previous for interpolation) -----------
double playerX = 5.0;
double prevPlayerX = 5.0;

// ----------- UPDATE (fixed timestep) -----------
void Update(double dt) {
    prevPlayerX = playerX;     // store old state for interpolation

    playerX += 20.0 * dt;      // move 20 cells per second

    if (playerX > 50)
        playerX = 0;
}

// ----------- RENDER (interpolated) -----------
void Render(int fps, int ups, double alpha) {
    erase();

    // interpolate position: smooth rendering
    double interpX = prevPlayerX * (1.0 - alpha) + playerX * alpha;

    mvprintw(0, 0, "FPS: %d   UPS: %d   alpha=%.2f", fps, ups, alpha);
    mvprintw(2, (int)interpX, "O");

    refresh();
}

int main() {
    initscr();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);

    auto previous = std::chrono::steady_clock::now();
    double accumulator = 0.0;

    // FPS/UPS counters
    int frames = 0, updates = 0;
    int fps = 0, ups = 0;
    auto timer = std::chrono::steady_clock::now();

    while (true) {
        // Input
        int key = getch();
        if (key == 'q') break;

        // Timing
        auto now = std::chrono::steady_clock::now();
        double frameTime = std::chrono::duration<double>(now - previous).count();
        previous = now;

        accumulator += frameTime;

        // ------------------- FIXED UPDATE -------------------
        while (accumulator >= FIXED_DELTA) {
            Update(FIXED_DELTA);
            accumulator -= FIXED_DELTA;
            updates++;
        }

        // Interpolation factor
        double alpha = accumulator / FIXED_DELTA;

        // ------------------- RENDER -------------------
        Render(fps, ups, alpha);
        frames++;

        // ------------------- FPS / UPS -------------------
        if (std::chrono::duration<double>(now - timer).count() >= 1.0) {
            fps = frames;
            ups = updates;
            frames = updates = 0;
            timer = now;
        }

        // small sleep reduces CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    endwin();
    return 0;
}

