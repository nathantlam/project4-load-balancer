/**
 * @file Request.h
 * @brief Data stored for one simulated web request.
 */
#ifndef REQUEST_H
#define REQUEST_H

#include <string>

/** @brief A job waiting for or running on a web server. */
struct Request {
    std::string ipIn;  ///< Requester's IPv4 address.
    std::string ipOut; ///< Destination IPv4 address.
    int time = 1;     ///< Required processing time in clock cycles; must be positive.
    char jobType = 'P'; ///< P for processing or S for streaming.
};

#endif
