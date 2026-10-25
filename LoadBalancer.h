/** @file LoadBalancer.h @brief FIFO queue, server ownership, scaling, and statistics. */
#ifndef LOAD_BALANCER_H
#define LOAD_BALANCER_H

#include "Config.h"
#include "IPFilter.h"
#include "Logger.h"
#include "RequestGenerator.h"
#include "WebServer.h"
#include <cstdint>
#include <memory>
#include <queue>
#include <string>
#include <vector>

/** @brief Counts jobs and capacity without storing a record per clock cycle. */
struct Statistics {
    std::uint64_t generated = 0; ///< Attempts including initialization.
    std::uint64_t rejected = 0; ///< Firewall rejections including initialization.
    std::uint64_t assigned = 0; ///< Jobs dispatched to servers.
    std::uint64_t completed = 0; ///< Finished jobs.
    std::uint64_t processingCompleted = 0; ///< Finished P jobs.
    std::uint64_t streamingCompleted = 0; ///< Finished S jobs.
    std::uint64_t busyServerCycles = 0; ///< Cycles spent processing across servers.
    std::uint64_t availableServerCycles = 0; ///< Sum of server counts each cycle.
    std::uint64_t initialGenerated = 0; ///< Attempts during queue initialization.
    std::uint64_t initialRejected = 0; ///< Blocked attempts during initialization.
    std::size_t startingQueue = 0; ///< Accepted initial jobs.
    std::size_t peakQueue = 0; ///< Largest waiting queue.
    int serversAdded = 0; ///< Servers added after setup.
    int serversRemoved = 0; ///< Servers deleted after setup.
    int minimumServers = 0; ///< Smallest observed server count.
    int maximumServers = 0; ///< Largest observed server count.
};

/** @brief Owns a FIFO job queue and dynamically allocated web servers. */
class LoadBalancer {
public:
    /** @brief Create a balancer. @param name Name used in logs.
     * @param initialServers Starting server count. @param config Validated settings.
     * @param logger Shared output destination, which must outlive this object. */
    LoadBalancer(const std::string& name, int initialServers,
                 const Config& config, Logger& logger);
    /** @brief Fill a queue of initialServers*100 accepted jobs.
     * @param generator Random workload source. @param type P/S to force a category, or A for both. */
    void fillInitialQueue(RequestGenerator& generator, char type = 'A');
    /** @brief Filter and enqueue a valid job. @param request Generated job.
     * @param cycle Current cycle. @return True when admitted. */
    bool addRequest(const Request& request, int cycle);
    /** @brief Dispatch, process, scale, and advance the local clock once. */
    void tick();
    /** @brief Print setup information and starting queue. */
    void printStartingSummary();
    /** @brief Print ending counters and accounting results. */
    void printSummary();
    /** @brief Get waiting jobs. @return Queue length. */
    std::size_t getQueueSize() const;
    /** @brief Get existing servers. @return Allocated server count. */
    std::size_t getServerCount() const;
    /** @brief Get running jobs. @return Busy server count. */
    std::size_t getBusyCount() const;
    /** @brief Read collected counters. @return Statistics reference. */
    const Statistics& getStatistics() const;
    /** @brief Check no accepted work is lost. @return True if all job counts agree. */
    bool accountingValid() const;
private:
    std::string name; ///< Log identifier.
    Config config; ///< Settings snapshot.
    Logger& logger; ///< Shared logger.
    IPFilter filter; ///< Incoming IP firewall.
    std::queue<Request> requests; ///< First-in, first-out waiting jobs.
    std::vector<std::unique_ptr<WebServer>> servers; ///< Owning server pointers.
    Statistics stats; ///< Measured counters.
    int initialServers; ///< Server count used to size the initial queue.
    int clock = 0; ///< Completed simulation cycles.
    int lastScaleCycle = 0; ///< Last actual server change.
    int nextServerId = 1; ///< Next unique identifier.
    bool initialized = false; ///< Prevent duplicate initialization.
    /** @brief Give waiting jobs to available servers. @param cycle Current cycle. */
    void assignRequests(int cycle);
    /** @brief Advance all busy servers. @param cycle Current cycle. */
    void processServers(int cycle);
    /** @brief Add/remove at most one server when eligible. @param cycle Current cycle. */
    void scaleServers(int cycle);
};

#endif
