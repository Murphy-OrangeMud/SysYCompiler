#include "logger.hpp"
#include <string>
#include <iostream>

void Logger::SetFunc(const std::string& funcName) {
#ifndef OJ
    function = funcName;
    std::cout << "[" << function << "]" << "Enter function: " << funcName << "\n";
#endif
}

void Logger::Error(const std::string& message) {
#ifndef OJ
    std::cout << "[" << function << "]" << "[Error]: " << message << "\n";
#endif
}

void Logger::Warn(const std::string& message) {
#ifndef OJ
    std::cout << "[" << function << "]" << "[Warning]: " << message << "\n";
#endif
}

void Logger::Info(const std::string& message) {
#ifndef OJ
    std::cout << "[" << function << "]" << "[Info]: " << message << "\n";
#endif
}

void Logger::UnSetFunc(const std::string& funcName) {
#ifndef OJ
    logger << "[" << function << "]" << "Leave function: " << funcName << "\n";
    function = funcName;
#endif
}