/** @file LoadBalancer.cpp @brief Queue dispatch, cycle processing, scaling, and reporting. */
#include "LoadBalancer.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

LoadBalancer::LoadBalancer(const std::string& name, int initialServers,
                           const Config& config, Logger& logger)
    : name(name), config(config), logger(logger), filter(config.blockedRanges),
      initialServers(initialServers) {
    config.validate();
    if (initialServers < config.minServers || initialServers > config.maxServers) {
        throw std::runtime_error("Starting servers must be within configured server limits");
    }
    if (filter.blocksAll()) {
        throw std::runtime_error("Firewall blocks every IPv4 address; cannot fill the initial queue");
    }
    for (int index = 0; index < initialServers; ++index) {
        servers.push_back(std::make_unique<WebServer>(nextServerId++));
    }
    stats.minimumServers = initialServers;
    stats.maximumServers = initialServers;
    // Allow the first scaling check at cycle 1, then wait a full cooldown.
    lastScaleCycle = -config.scalingCooldown;
}

void LoadBalancer::fillInitialQueue(RequestGenerator& generator, char type) {
    if (initialized || stats.generated != 0 || clock != 0) {
        throw std::runtime_error("Initial queue must be filled exactly once before simulation");
    }
    if (type != 'A' && type != 'P' && type != 'S') {
        throw std::runtime_error("Invalid initial job type");
    }
    const std::size_t target = static_cast<std::size_t>(initialServers) * 100;
    // A nearly complete firewall can otherwise leave initialization running indefinitely.
    const std::uint64_t attemptLimit = std::max<std::uint64_t>(1000000, target * 1000ULL);
    while (requests.size() < target) {
        if (stats.generated >= attemptLimit) {
            throw std::runtime_error("Too many blocked setup requests; narrow the blocked IP ranges");
        }
        Request request = generator.generate();
        if (type != 'A') request.jobType = type;
        addRequest(request, 0);
    }
    stats.startingQueue = requests.size();
    stats.initialGenerated = stats.generated;
    stats.initialRejected = stats.rejected;
    initialized = true;
}

bool LoadBalancer::addRequest(const Request& request, int cycle) {
    if (request.time < 1 || (request.jobType != 'P' && request.jobType != 'S')) {
        throw std::runtime_error("Invalid request duration or job type");
    }
    // Validate/match the source before changing counters.
    const bool blocked = filter.isBlocked(request.ipIn);
    ++stats.generated;
    if (blocked) {
        ++stats.rejected;
        logger.event(cycle, name, "BLOCK", "rejected source=" + request.ipIn);
        return false;
    }
    requests.push(request);
    stats.peakQueue = std::max(stats.peakQueue, requests.size());
    logger.event(cycle, name, "ARRIVAL", request.ipIn + " -> " + request.ipOut +
                 " type=" + request.jobType + " time=" + std::to_string(request.time) +
                 " queue=" + std::to_string(requests.size()));
    return true;
}

void LoadBalancer::assignRequests(int cycle) {
    for (auto& server : servers) {
        if (requests.empty()) break;
        if (!server->isBusy()) {
            if (!server->assignRequest(requests.front())) {
                throw std::logic_error("Idle server rejected a queued request");
            }
            requests.pop();
            ++stats.assigned;
            logger.event(cycle, name, "ASSIGN", "server=" + std::to_string(server->getId()) +
                         " type=" + server->getCurrentRequest().jobType +
                         " time=" + std::to_string(server->getRemainingTime()));
        }
    }
}

void LoadBalancer::processServers(int cycle) {
    stats.availableServerCycles += servers.size();
    for (auto& server : servers) {
        if (server->isBusy()) ++stats.busyServerCycles;
        if (server->processCycle()) {
            ++stats.completed;
            if (server->getCurrentRequest().jobType == 'P') ++stats.processingCompleted;
            else ++stats.streamingCompleted;
            logger.event(cycle, name, "COMPLETE", "server=" + std::to_string(server->getId()));
        }
    }
}

void LoadBalancer::scaleServers(int cycle) {
    if (static_cast<long long>(cycle) - lastScaleCycle < config.scalingCooldown) return;
    if (requests.size() > 80 * servers.size() &&
        servers.size() < static_cast<std::size_t>(config.maxServers)) {
        const int id = nextServerId++;
        servers.push_back(std::make_unique<WebServer>(id));
        ++stats.serversAdded;
        stats.maximumServers = std::max(stats.maximumServers, static_cast<int>(servers.size()));
        lastScaleCycle = cycle;
        logger.event(cycle, name, "SCALE", "added server=" + std::to_string(id) +
                     " servers=" + std::to_string(servers.size()) +
                     " queue=" + std::to_string(requests.size()));
    } else if (requests.size() < 50 * servers.size() &&
               servers.size() > static_cast<std::size_t>(config.minServers)) {
        const auto idle = std::find_if(servers.begin(), servers.end(),
            [](const std::unique_ptr<WebServer>& server) { return !server->isBusy(); });
        if (idle == servers.end()) return; // Retry later without interrupting a running job.
        const int id = (*idle)->getId();
        servers.erase(idle); // unique_ptr frees the idle server immediately.
        ++stats.serversRemoved;
        stats.minimumServers = std::min(stats.minimumServers, static_cast<int>(servers.size()));
        lastScaleCycle = cycle;
        logger.event(cycle, name, "SCALE", "removed server=" + std::to_string(id) +
                     " servers=" + std::to_string(servers.size()) +
                     " queue=" + std::to_string(requests.size()));
    }
}

void LoadBalancer::tick() {
    if (!initialized) throw std::logic_error("Fill the initial queue before running");
    const int cycle = clock + 1;
    assignRequests(cycle);
    processServers(cycle);
    scaleServers(cycle);
    ++clock;
}

std::size_t LoadBalancer::getQueueSize() const { return requests.size(); }
std::size_t LoadBalancer::getServerCount() const { return servers.size(); }
std::size_t LoadBalancer::getBusyCount() const {
    return static_cast<std::size_t>(std::count_if(servers.begin(), servers.end(),
        [](const std::unique_ptr<WebServer>& server) { return server->isBusy(); }));
}
const Statistics& LoadBalancer::getStatistics() const { return stats; }

bool LoadBalancer::accountingValid() const {
    const auto accepted = stats.generated - stats.rejected;
    return accepted == stats.completed + requests.size() + getBusyCount() &&
           stats.assigned == stats.completed + getBusyCount() &&
           stats.completed == stats.processingCompleted + stats.streamingCompleted &&
           static_cast<std::size_t>(initialServers + stats.serversAdded - stats.serversRemoved) == servers.size();
}

void LoadBalancer::printStartingSummary() {
    logger.summary("\n--- " + name + " starting status ---");
    logger.summary("Starting servers: " + std::to_string(initialServers));
    logger.summary("Starting queue size: " + std::to_string(stats.startingQueue));
    logger.summary("Task time range: " + std::to_string(config.minTaskTime) + "-" +
                   std::to_string(config.maxTaskTime) + " clock cycles");
    logger.summary("Initialization generated / rejected: " + std::to_string(stats.initialGenerated) +
                   " / " + std::to_string(stats.initialRejected));
}

void LoadBalancer::printSummary() {
    logger.summary("\n--- " + name + " ending status ---");
    logger.summary("Clock cycles completed: " + std::to_string(clock));
    logger.summary("Starting queue size: " + std::to_string(stats.startingQueue));
    logger.summary("Ending queue size: " + std::to_string(requests.size()));
    logger.summary("Peak queue size: " + std::to_string(stats.peakQueue));
    logger.summary("Task time range: " + std::to_string(config.minTaskTime) + "-" +
                   std::to_string(config.maxTaskTime) + " clock cycles");
    logger.summary("Generated requests (including setup): " + std::to_string(stats.generated));
    logger.summary("Accepted requests (including setup): " + std::to_string(stats.generated - stats.rejected));
    logger.summary("New accepted requests after setup: " +
                   std::to_string(stats.generated - stats.rejected - stats.startingQueue));
    logger.summary("Rejected / blocked requests (including setup): " + std::to_string(stats.rejected));
    logger.summary("Discarded accepted requests: 0");
    logger.summary("Assigned requests: " + std::to_string(stats.assigned));
    logger.summary("Completed requests: " + std::to_string(stats.completed));
    logger.summary("Completed processing / streaming: " + std::to_string(stats.processingCompleted) +
                   " / " + std::to_string(stats.streamingCompleted));
    logger.summary("In-service requests: " + std::to_string(getBusyCount()));
    logger.summary("Ending total servers: " + std::to_string(servers.size()));
    logger.summary("Active (busy) servers: " + std::to_string(getBusyCount()));
    logger.summary("Inactive (idle) servers: " + std::to_string(servers.size() - getBusyCount()));
    logger.summary("Servers added / removed: " + std::to_string(stats.serversAdded) + " / " +
                   std::to_string(stats.serversRemoved));
    logger.summary("Minimum / maximum servers: " + std::to_string(stats.minimumServers) + " / " +
                   std::to_string(stats.maximumServers));
    std::ostringstream utilization;
    utilization << std::fixed << std::setprecision(2)
                << (stats.availableServerCycles == 0 ? 0.0 :
                    100.0 * stats.busyServerCycles / stats.availableServerCycles);
    logger.summary("Server utilization: " + utilization.str() + "%");
    logger.summary("Request and server accounting: " + std::string(accountingValid() ? "PASS" : "FAIL"));
}
