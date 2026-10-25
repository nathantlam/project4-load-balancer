/** @file WebServer.cpp @brief Basic web-server state and processing methods. */
#include "WebServer.h"

WebServer::WebServer(int id) : id(id) {
}

int WebServer::getId() const {
    return id;
}

bool WebServer::isBusy() const {
    return busy;
}

int WebServer::getRemainingTime() const {
    return remainingTime;
}

const Request& WebServer::getCurrentRequest() const {
    return currentRequest;
}

bool WebServer::assignRequest(const Request& request) {
    if (busy || request.time <= 0 ||
        (request.jobType != 'P' && request.jobType != 'S')) {
        return false;
    }

    currentRequest = request;
    remainingTime = request.time;
    busy = true;
    return true;
}

bool WebServer::processCycle() {
    if (!busy) {
        return false;
    }

    --remainingTime;
    if (remainingTime == 0) {
        busy = false;
        return true;
    }

    return false;
}
