#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <stdexcept>
#include <unistd.h> // for pid_t
#include <mutex>

class Logger {
public:
    enum class LogLevel { LOG, INFO, WARNING, ERROR };

    // Constructor: provide log file name
    Logger(const std::string& filename);

    // Log a message with PID and level
    void log(const std::string& msg, pid_t pid_, LogLevel lv = LogLevel::LOG, bool Console_output_ = true);

private:
    std::ofstream logFile;
    std::mutex logMutex; // protect writes

    // Helper to get current timestamp
    std::string getCurrentTime() const;
};

#endif // LOGGER_H
