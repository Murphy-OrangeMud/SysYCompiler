#ifndef LOGGER_INCLUDED
#define LOGGER_INCLUDED

#include <iostream>
#include <fstream>

class Logger {
private:
    std::ofstream logger;
    std::string function;
    std::string path;
public:
    Logger() = default;
    explicit Logger(const std::string& _path) {
        path = _path;
    }

    bool open() {
#ifndef OJ
        logger.open(path, std::ios::out);
        if (!logger) {
            std::cerr << "Cannot open log file: " << path << std::endl;
            return false;
        }
#endif
        return true;
    }

    Logger &operator = (const Logger & other) {
        this->path = other.path;
        return *this;
    }

    void SetFunc(const std::string& funcName);

    void UnSetFunc(const std::string& funcName);

    void Warn(const std::string& message);

    void Info(const std::string& message);

    void Error(const std::string& message);

    ~Logger() {
#ifndef OJ
        logger.close();
#endif
    }
};

#endif