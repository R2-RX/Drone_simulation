#include <iostream>
#include <chrono>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <stdexcept>
#include "BlackBoard.h"
#include "Pipe.h"
#include "Logger.h"

// ------------------------- Globals -------------------------
Logger logger(SYSTEM_WIDE_LOG);
BlackBoard blackboard;

static bool shutdownFlage = false; // plain bool

// ---------------------- Signal Handlers ----------------------

// Signal handler called by POSIX timer
void timer_handler(int /*signum*/) {
    auto now = std::chrono::system_clock::now();
    int64_t ns_since_epoch = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    blackboard.setGlobalTime(ns_since_epoch);
}

// SIGTERM handler
void handle_sigterm(int /*signum*/) {
    shutdownFlage = true;
}

// ------------------------- Main -------------------------
int main() {
    try {
        // ------------------ SIGTERM ------------------
        struct sigaction sa_term{};
        sa_term.sa_handler = handle_sigterm;
        sa_term.sa_flags = 0;
        sigemptyset(&sa_term.sa_mask);
        if (sigaction(SIGTERM, &sa_term, nullptr) == -1) {
            throw std::runtime_error("Failed to setup SIGTERM handler");
        }

        // ------------------ Timer ------------------
        struct sigaction sa_timer{};
        sa_timer.sa_handler = timer_handler;
        sa_timer.sa_flags = 0;
        sigemptyset(&sa_timer.sa_mask);
        if (sigaction(SIGRTMIN, &sa_timer, nullptr) == -1) {
            throw std::runtime_error("Failed to setup SIGRTMIN handler");
        }

        // Create POSIX timer
        timer_t timerid;
        struct sigevent sev{};
        sev.sigev_notify = SIGEV_SIGNAL;
        sev.sigev_signo = SIGRTMIN;

        if (timer_create(CLOCK_MONOTONIC, &sev, &timerid) == -1) {
            throw std::runtime_error("Failed to create timer");
        }

        // Timer to fire every 1 ms
        struct itimerspec its{};
        its.it_value.tv_sec = 0;
        its.it_value.tv_nsec = 1'000'000; // first expiration 1 ms
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 1'000'000; // repeat every 1 ms

        if (timer_settime(timerid, 0, &its, nullptr) == -1) {
            throw std::runtime_error("Failed to start timer");
        }

        blackboard.setProcessPid(WatchDogProcName::GlobalTimer_Proc, getpid());

        // ------------------ Pipe ------------------
        Pipe<char> globaltimer_pipe_wd(GLOBALTIMER_PIPE_WD);

        // ------------------ Main loop ------------------
        const int HEARTBEAT_MS = 10; // send heartbeat every 10 ms
        while (!shutdownFlage) {
            globaltimer_pipe_wd.send_data('T');

            // Sleep until next heartbeat or signal
            struct timespec ts{};
            ts.tv_sec = HEARTBEAT_MS / 1000;
            ts.tv_nsec = (HEARTBEAT_MS % 1000) * 1'000'000;

            nanosleep(&ts, nullptr); // can be interrupted by signals
        }

        logger.log("Received SIGTERM, shutting down...", getpid(), Logger::LogLevel::WARNING);

    } catch (const std::exception& e) {
        std::string msg = e.what();
        logger.log("Fatal error in GlobalTimer: " + msg, getpid(), Logger::LogLevel::ERROR);
        return 1;
    }

    return 0;
}
