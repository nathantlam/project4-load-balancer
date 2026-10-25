/** @file Switch.h @brief Optional job-type router for two independent balancers. */
#ifndef SWITCH_H
#define SWITCH_H

#include "LoadBalancer.h"

/** @brief Routes P/S requests to separate queues and server pools. */
class Switch {
public:
    /** @brief Create two pools. @param servers Initial servers per pool.
     * @param config Settings. @param logger Shared output. */
    Switch(int servers, const Config& config, Logger& logger);
    /** @brief Fill both starting queues. @param generator Shared workload source. */
    void fillInitialQueues(RequestGenerator& generator);
    /** @brief Route and filter one job. @param request Job to route.
     * @param cycle Current cycle. @return True if admitted. */
    bool routeRequest(const Request& request, int cycle);
    /** @brief Advance both independent pools for the same simulated cycle. */
    void tick();
    /** @brief Print each pool's starting status. */
    void printStartingSummary();
    /** @brief Print each pool's separate final statistics. */
    void printSummary();
    /** @brief Verify both pools' accounting. @return True if both agree. */
    bool accountingValid() const;
private:
    LoadBalancer processingLB; ///< Pool dedicated to P jobs.
    LoadBalancer streamingLB; ///< Pool dedicated to S jobs.
};

#endif
