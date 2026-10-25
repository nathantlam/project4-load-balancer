/**
 * @file WebServer.h
 * @brief Interface for a server that processes one request at a time.
 */
#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "Request.h"

/** @brief Holds one active request and advances its processing each cycle. */
class WebServer {
public:
    /** @brief Create an idle server. @param id Unique server identifier. */
    explicit WebServer(int id);

    /** @brief Return this server's identifier. @return Server ID. */
    int getId() const;

    /** @brief Check whether a request is running. @return True when busy. */
    bool isBusy() const;

    /** @brief Return remaining processing time. @return Cycles left, or zero if idle. */
    int getRemainingTime() const;

    /** @brief Read the most recently assigned job. @return Stored request data. */
    const Request& getCurrentRequest() const;

    /**
     * @brief Assign work to an idle server.
     * @param request Job with positive time and type P or S.
     * @return True if accepted; false if busy or the job is invalid.
     */
    bool assignRequest(const Request& request);

    /**
     * @brief Process one cycle without assigning another request.
     * @return True only on the cycle when the current request finishes.
     */
    bool processCycle();

private:
    int id;                 ///< Unique identifier.
    bool busy = false;      ///< Whether this server holds active work.
    Request currentRequest; ///< Most recently assigned request.
    int remainingTime = 0;  ///< Cycles left for active work.
};

#endif
