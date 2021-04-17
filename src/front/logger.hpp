#ifndef LOGGER_INCLUDED
#define LOGGER_INCLUDED

#include <iostream>
#include <fstream>

class Logger {
private:
    ofstream logger;
    std::string function;
public:
    Logger(const std::string path) {
#ifndef OJ
        logger.open(path, ios::out);
        if (!logger) {
            std::cerr << "Cannot open log file: " << path << std::endl;
            return;
        }
#endif
    }
    void SetFunc(std::string funcName);
    void UnSetFunc(std::string funcName);
    void Warning(std::string message);
    void Info(std::string message);
    void Error(std::string message);
    ~Logger() {
#ifndef OJ
        logger.close();
#endif
    }
};

#endif