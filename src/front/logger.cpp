#include "logger.hpp"
#include <string>
#include <iostream>

void Logger::SetFunc(const std::string& funcName) {
#ifndef OJ
    function = funcName;
    std::cerr << "[" << function << "]" << "Enter function: " << funcName << "\n";
#endif
}

void Logger::Error(const std::string& message) {
#ifndef OJ
    std::cerr << "[" << function << "]" << "[Error]: " << message << "\n";
#endif
}

void Logger::Warn(const std::string& message) {
#ifndef OJ
    std::cerr << "[" << function << "]" << "[Warning]: " << message << "\n";
#endif
}

void Logger::Info(const std::string& message) {
#ifndef OJ
    std::cerr << "[" << function << "]" << "[Info]: " << message << "\n";
#endif
}

void Logger::UnSetFunc(const std::string& funcName) {
#ifndef OJ
    std::cerr << "[" << function << "]" << "Leave function: " << function  << " to " << funcName << "\n";
    function = funcName;
#endif
}