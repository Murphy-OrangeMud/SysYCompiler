#include "logger.hpp"

void Logger::SetFunc(std::string funcName) {
    function = funcName;
    logger << "[" << function << "]" << "Enter function: " << funcName << "\n";
}

void Logger::Error(std::string message) {
    logger << "[" << function << "]" << "-Error: " << message << "\n";
}

void Logger::Warning(std::string message) {
    logger << "[" << function << "]" << "-Warning: " << message << "\n";
}

void Logger::Info(std::string message) {
    logger << "[" << function << "]" << "-Info: " << message << "\n";
}

void Logger::UnSetFunc(std::string funcName) {
    logger << "[" << function << "]" << "Leave function: " << funcName << "\n";
    function = funcName;
}