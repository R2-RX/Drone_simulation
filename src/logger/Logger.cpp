#include "Logger.h"

Logger::Logger(const std::string& filename) : logFile(filename, std::ios::app) {
    if (!logFile.is_open())
        throw std::runtime_error("Cannot open log file: " + filename);
}

void Logger::log(const std::string& msg, pid_t pid_, LogLevel lv, bool Console_output_) {
    std::lock_guard<std::mutex> lock(logMutex); // lock while writing
    
    const char* prefix;
    const char* colorCode;

    switch (lv) {
        case LogLevel::INFO:    prefix = "INFO";    colorCode = "\033[32m"; break; // Green
        case LogLevel::WARNING: prefix = "WARNING"; colorCode = "\033[33m"; break; // Yellow
        case LogLevel::ERROR:   prefix = "ERROR";   colorCode = "\033[31m"; break; // Red
        default:                prefix = "LOG";    colorCode = "\033[0m";  break; // Default
    }

    std::string timestamp = getCurrentTime();

    // Console output with color
    if (Console_output_)
        std::cout << timestamp << " [" << colorCode << prefix << "\033[0m" << "] "
                << "pid: " << pid_ << "|->" << msg << std::endl;

    // File output without color
    logFile << timestamp << " [" << prefix << "] "
            << "pid: " << pid_ << "|->" << msg << std::endl;
    logFile.flush(); // read immediately
}

std::string Logger::getCurrentTime() const {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buffer);
}
