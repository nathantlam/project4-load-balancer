/** @file main.cpp @brief User input, configuration, and simulation driver. */
#include "Config.h"
#include "LoadBalancer.h"
#include "Logger.h"
#include "RequestGenerator.h"
#include "Switch.h"

#include <iostream>
#include <filesystem>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
/** @brief Read a bounded whole number. @param value Text. @param label Field name.
 * @param maximum Largest allowed number. @param minimum Smallest allowed number.
 * @return Parsed number. */
long long readNumber(const std::string& value, const std::string& label,
                     long long maximum, long long minimum = 1) {
    std::istringstream input(value);
    long long number = 0;
    if (!(input >> number)) throw std::runtime_error("Invalid " + label);
    input >> std::ws;
    if (!input.eof() || number < minimum || number > maximum) {
        throw std::runtime_error(label + " must be between " + std::to_string(minimum) +
                                 " and " + std::to_string(maximum));
    }
    return number;
}

/** @brief Prompt until a valid positive integer is entered. @param prompt User message.
 * @param maximum Input limit. @return User's number. */
int promptNumber(const std::string& prompt, int maximum) {
    std::string line;
    while (true) {
        std::cout << prompt;
        if (!std::getline(std::cin, line)) throw std::runtime_error("Input ended before setup was complete");
        try { return static_cast<int>(readNumber(line, "input", maximum)); }
        catch (const std::exception& error) { std::cout << error.what() << '\n'; }
    }
}

/** @brief Explain command-line arguments and bonus server-count semantics. */
void printHelp() {
    std::cout << "Aggie Cloud Solutions - load balancer\n"
              << "Usage: ./loadbalancer [options]\n"
              << "  --servers N     Starting servers (prompts when omitted)\n"
              << "  --cycles N      Clock cycles, 1-1000000000 (prompts when omitted)\n"
              << "  --config PATH   Settings file (default: config.txt)\n"
              << "  --log PATH      Plain log file (default: simulation.log)\n"
              << "  --seed N        Override configured seed; zero is allowed\n"
              << "  --quiet         Starting/ending summaries only (default)\n"
              << "  --verbose       Include changing events, not per-cycle status\n"
              << "  --color         ANSI colors for console events only\n"
              << "  --no-color      Plain console text (default)\n"
              << "  --bonus         Separate P/S balancers; N servers PER pool\n"
              << "  --help          Show this help\n";
}
}

/** @brief Run the simulation. @param argc Argument count. @param argv Arguments.
 * @return Zero on success, one on input/output errors, two on failed accounting. */
int main(int argc, char* argv[]) {
    try {
        int servers = 0;
        int cycles = 0;
        std::string configPath = "config.txt";
        std::string logPath = "simulation.log";
        bool verbose = false;
        bool color = false;
        bool bonus = false;
        bool seedOverride = false;
        unsigned int seed = 0;
        for (int index = 1; index < argc; ++index) {
            const std::string option = argv[index];
            if (option == "--help") { printHelp(); return 0; }
            if (option == "--verbose") verbose = true;
            else if (option == "--quiet") verbose = false;
            else if (option == "--color") color = true;
            else if (option == "--no-color") color = false;
            else if (option == "--bonus") bonus = true;
            else if (option == "--servers" || option == "--cycles" || option == "--config" ||
                     option == "--log" || option == "--seed") {
                if (++index >= argc) throw std::runtime_error("Missing value for " + option);
                const std::string value = argv[index];
                if (option == "--servers") servers = static_cast<int>(readNumber(value, "servers", 10000));
                else if (option == "--cycles") cycles = static_cast<int>(readNumber(value, "cycles", 1000000000));
                else if (option == "--config") configPath = value;
                else if (option == "--log") logPath = value;
                else {
                    seed = static_cast<unsigned int>(readNumber(value, "seed",
                           std::numeric_limits<unsigned int>::max(), 0));
                    seedOverride = true;
                }
            } else throw std::runtime_error("Unknown option: " + option + "; use --help");
        }

        Config config = Config::load(configPath);
        if (seedOverride) config.randomSeed = seed;
        if (servers == 0) servers = promptNumber("Starting servers (per pool in bonus mode): ", 10000);
        if (cycles == 0) cycles = promptNumber("Clock cycles to run: ", 1000000000);
        if (servers < config.minServers || servers > config.maxServers) {
            throw std::runtime_error("Starting servers must be within configured server limits");
        }
        // Validate the firewall before opening/truncating the output file.
        IPFilter filter(config.blockedRanges);
        if (filter.blocksAll()) throw std::runtime_error("Firewall blocks all IPv4 addresses");
        if (std::filesystem::weakly_canonical(logPath) == std::filesystem::weakly_canonical(configPath)) {
            throw std::runtime_error("Log and config must use different files");
        }

        Logger logger(logPath, verbose, color);
        RequestGenerator generator(config);
        logger.summary("Aggie Cloud Solutions - Load Balancer Simulation");
        logger.summary("Mode: " + std::string(verbose ? "verbose" : "quiet"));
        logger.summary("Requested clock cycles: " + std::to_string(cycles));
        logger.summary("Random seed: " + std::to_string(config.randomSeed));
        logger.summary("Arrival probability per cycle: " + std::to_string(config.arrivalProbability));
        logger.summary("Arrival batch range: " + std::to_string(config.minArrivals) + "-" + std::to_string(config.maxArrivals));
        logger.summary("Scaling: add above 80 * servers; remove below 50 * servers");
        logger.summary("Scaling cooldown: " + std::to_string(config.scalingCooldown) + " cycles");
        logger.summary("Server limits per balancer: " + std::to_string(config.minServers) + "-" + std::to_string(config.maxServers));
        logger.summary("Blocked incoming IP ranges:");
        if (config.blockedRanges.empty()) logger.summary("  none");
        for (const auto& range : config.blockedRanges) logger.summary("  " + range);

        std::unique_ptr<LoadBalancer> balancer;
        std::unique_ptr<Switch> router;
        if (bonus) {
            logger.summary("Bonus: P/S pools advance together; starting servers apply to EACH pool");
            router = std::make_unique<Switch>(servers, config, logger);
            router->fillInitialQueues(generator);
            router->printStartingSummary();
        } else {
            balancer = std::make_unique<LoadBalancer>("Main", servers, config, logger);
            balancer->fillInitialQueue(generator);
            balancer->printStartingSummary();
        }

        for (int cycle = 1; cycle <= cycles; ++cycle) {
            const int arrivals = generator.arrivalsThisCycle();
            for (int index = 0; index < arrivals; ++index) {
                const Request request = generator.generate();
                if (bonus) router->routeRequest(request, cycle);
                else balancer->addRequest(request, cycle);
            }
            if (bonus) router->tick();
            else balancer->tick();
        }

        if (bonus) router->printSummary();
        else balancer->printSummary();
        const bool valid = bonus ? router->accountingValid() : balancer->accountingValid();
        logger.summary("\nSimulation status: " + std::string(valid ? "COMPLETE" : "ACCOUNTING ERROR"));
        logger.summary("No extra cycles were used to drain unfinished jobs.");
        return valid ? 0 : 2;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
