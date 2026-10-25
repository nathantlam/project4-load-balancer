/** @file Logger.cpp @brief Event and summary output. */
#include "Logger.h"

#include <iostream>
#include <stdexcept>

Logger::Logger(const std::string& path, bool verbose, bool color)
    : output(path), verbose(verbose), color(color) {
    if (!output) throw std::runtime_error("Cannot write log file: " + path);
}

void Logger::summary(const std::string& message) {
    std::cout << message << '\n';
    output << message << '\n';
    if (!output) throw std::runtime_error("Failed writing log file");
}

void Logger::event(int cycle, const std::string& source, const std::string& kind,
                   const std::string& message) {
    if (!verbose) return;
    const std::string line = "[cycle " + std::to_string(cycle) + "][" + source +
                             "][" + kind + "] " + message;
    output << line << '\n';
    if (!output) throw std::runtime_error("Failed writing log file");
    if (color) {
        const char* code = "\033[36m";
        if (kind == "BLOCK") {
            code = "\033[31m";
        } else if (kind == "SCALE") {
            code = "\033[33m";
        }
        std::cout << code << line << "\033[0m\n";
    } else {
        std::cout << line << '\n';
    }
}
