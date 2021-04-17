#include "logger.hpp"
#include <string>
#include <iostream>

void Logger::SetFunc(std::string funcName) {
#ifndef OJ
    function = funcName;
    logger << "[" << function << "]" << "Enter function: " << funcName << "\n";
#endif
}

void Logger::Error(std::string message) {
#ifndef OJ
    logger << "[" << function << "]" << "-Error: " << message << "\n";
#endif
}

void Logger::Warning(std::string message) {
#ifndef OJ
    logger << "[" << function << "]" << "-Warning: " << message << "\n";
#endif
}

void Logger::Info(std::string message) {
#ifndef OJ
    logger << "[" << function << "]" << "-Info: " << message << "\n";
#endif
}

void Logger::UnSetFunc(std::string funcName) {
#ifndef OJ
    logger << "[" << function << "]" << "Leave function: " << funcName << "\n";
    function = funcName;
#endif
}