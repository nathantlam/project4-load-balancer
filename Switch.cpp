/** @file Switch.cpp @brief Job routing and concurrent simulated pool advancement. */
#include "Switch.h"
#include <stdexcept>

Switch::Switch(int servers, const Config& config, Logger& logger)
    : processingLB("Processing", servers, config, logger),
      streamingLB("Streaming", servers, config, logger) {
}

void Switch::fillInitialQueues(RequestGenerator& generator) {
    processingLB.fillInitialQueue(generator, 'P');
    streamingLB.fillInitialQueue(generator, 'S');
}

bool Switch::routeRequest(const Request& request, int cycle) {
    if (request.jobType == 'P') {
        return processingLB.addRequest(request, cycle);
    }
    if (request.jobType == 'S') {
        return streamingLB.addRequest(request, cycle);
    }
    throw std::runtime_error("Switch requires a P or S request");
}

void Switch::tick() {
    processingLB.tick();
    streamingLB.tick();
}

void Switch::printStartingSummary() {
    processingLB.printStartingSummary();
    streamingLB.printStartingSummary();
}
void Switch::printSummary() {
    processingLB.printSummary();
    streamingLB.printSummary();
}

bool Switch::accountingValid() const {
    return processingLB.accountingValid() && streamingLB.accountingValid();
}
