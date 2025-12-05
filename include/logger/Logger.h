#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>
#include <sstream>
#include <ctime>
#include <iomanip>

class Logger {
public:
    enum class LogLevel { LOG, INFO, WARNING, ERROR };

private:
    LogLevel level = LogLevel::LOG;

public:
    void setLevel(LogLevel lv) { level = lv; }

    void log(const std::string& msg, pid_t pid_, LogLevel lv = LogLevel::LOG);

private:
    // Helper function to get current time as string
    std::string getCurrentTime() const;
};

#endif // LOGGER_H
