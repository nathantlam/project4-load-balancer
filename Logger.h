/** @file Logger.h @brief Plain log files and optional colored console events. */
#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <string>

/** @brief Writes summaries in both modes and events only in verbose mode. */
class Logger {
public:
    /** @brief Open the log. @param path Output file. @param verbose Include events.
     * @param color Use ANSI colors on the console only. */
    Logger(const std::string& path, bool verbose, bool color);
    /** @brief Write a summary line. @param message Text for file and console. */
    void summary(const std::string& message);
    /** @brief Write a changing event. @param cycle Simulation cycle (0 means setup).
     * @param source Balancer name. @param kind Event category. @param message Details. */
    void event(int cycle, const std::string& source, const std::string& kind,
               const std::string& message);
private:
    std::ofstream output; ///< Plain-text output file.
    bool verbose; ///< Include individual events.
    bool color; ///< Color terminal events.
};

#endif
