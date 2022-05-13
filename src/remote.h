/*
 * remote.h
 *
 *  Created on: Feb 25, 2016
 *      Author: eba
 */

#ifndef REMOTE_H_
#define REMOTE_H_

#include <string>
#include <map>
#include <atomic>

void init_remote();

typedef std::map<std::string, std::string> headers_t;
typedef std::map<std::string, std::string> params_t;
typedef std::atomic<bool> canceller_t;

struct response_t {
    std::string response;
    long response_code;
};

response_t post(std::string url, std::string payload, const headers_t & headers,
                int timeout_seconds = -1, canceller_t * canceller = nullptr);

response_t get(std::string url, const params_t & params, int timeout_seconds = -1, canceller_t * canceller = nullptr,
               const char * pubkey = nullptr);

void cleanup_remote();

#endif /* REMOTE_H_ */
