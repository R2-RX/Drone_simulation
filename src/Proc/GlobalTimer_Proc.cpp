// #include <chrono>
// #include <iostream>
// #include "BlackBoard.h"
// #include "thread"

// int main() {
//     BlackBoard blackboard;

//     // auto sim_start = std::chrono::steady_clock::now();

//     while (true) {

//         //debug print every ~100ms
//         // auto sim_now = std::chrono::steady_clock::now();
//         // int64_t ns_elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(sim_now - sim_start).count();

//         // if (ns_elapsed % 100'000'000 < 1000) {
//         //     std::cout << "Elapsed ns: " << ns_elapsed << "\n";
//         // }

//         // timestamp
//         auto abs_now = std::chrono::system_clock::now();
//         int64_t ns_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(
//             abs_now.time_since_epoch()).count();

//         blackboard.setGlobalTime(ns_since_epoch);
//               // std::this_thread::sleep_for(std::chrono::milliseconds(1)); // for cpu usage reduction 

//     }

//     return 0;
// }


#include <iostream>
#include <chrono>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "BlackBoard.h"

BlackBoard blackboard;

// Signal handler called by POSIX timer
void timer_handler(int /*signum*/) {
    auto now = std::chrono::system_clock::now();
    int64_t ns_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    blackboard.setGlobalTime(ns_since_epoch);
}

int main() {
    // Setup signal action
    struct sigaction sa{};
    sa.sa_handler = timer_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGRTMIN, &sa, nullptr) == -1) {
        perror("sigaction");
        return 1;
    }

    // Create the timer
    timer_t timerid;
    struct sigevent sev{};
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;

    if (timer_create(CLOCK_MONOTONIC, &sev, &timerid) == -1) {
        perror("timer_create");
        return 1;
    }

    // Set timer to fire every 1 ms
    struct itimerspec its{};
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 1'000'000; // first expiration: 1 ms
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 1'000'000; // repeat every 1 ms

    if (timer_settime(timerid, 0, &its, nullptr) == -1) {
        perror("timer_settime");
        return 1;
    }

    // Keep main thread alive
    while (true) {
        pause(); // sleep until signal arrives
    }

    return 0;
}
