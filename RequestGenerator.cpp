/** @file RequestGenerator.cpp @brief Random request and arrival generation. */
#include "RequestGenerator.h"

RequestGenerator::RequestGenerator(const Config& config)
    : engine(config.randomSeed), taskTime(config.minTaskTime, config.maxTaskTime),
      batchSize(config.minArrivals, config.maxArrivals), arrival(config.arrivalProbability) {
}

std::string RequestGenerator::randomIP() {
    std::string address;
    for (int index = 0; index < 4; ++index) {
        if (index != 0) address += '.';
        address += std::to_string(octet(engine));
    }
    return address;
}

Request RequestGenerator::generate() {
    Request request;
    request.ipIn = randomIP();
    request.ipOut = randomIP();
    request.time = taskTime(engine);
    request.jobType = jobType(engine) == 0 ? 'P' : 'S';
    return request;
}

int RequestGenerator::arrivalsThisCycle() {
    return arrival(engine) ? batchSize(engine) : 0;
}
