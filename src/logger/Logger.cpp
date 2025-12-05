#include "Logger.h"   
#include <iostream>    
#include <iomanip>     
#include <ctime>      
#include <sstream>     
#include "ItemData.h"

void Logger::log(const std::string& msg, pid_t pid_, LogLevel lv) {
    if (lv < level) return;

    const char* prefix;
    const char* colorCode;

    switch (lv) {
        case LogLevel::INFO:    
            prefix = "INFO";    
            colorCode = "\033[32m"; // Green
            break;
        case LogLevel::WARNING: 
            prefix = "WARNING"; 
            colorCode = "\033[33m"; // Yellow
            break;
        case LogLevel::ERROR:   
            prefix = "ERROR";   
            colorCode = "\033[31m"; // Red
            break;
        default:
            prefix = "LOG";
            colorCode = "\033[0m"; // Default
            break;
    }

    std::cout << getCurrentTime() << " [" << colorCode << prefix << "\033[0m" << "] " << "pid: "  << pid_ << "|->" << msg << std::endl;
}


std::string Logger::getCurrentTime() const {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss; // String stream to format the time
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}
