/*
 * remote.cpp
 *
 *  Created on: Feb 25, 2016
 *      Author: eba
 */

#include <vector>

#include <remote.h>

#include <curl/curl.h>

size_t write_to_vector_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t realsize = size * nmemb;
    auto vec = (std::vector<char> *)userdata;
    for(size_t i=0; i<realsize; i++) vec->push_back(ptr[i]);
    return realsize;
}

int canceller_callback(void *clientp,   double dltotal,   double dlnow,   double ultotal,   double ulnow) {
    bool cancel = *((canceller_t *) clientp);
    return cancel;
}

response_t perform_request(CURL * curl, std::string url, int timeout_seconds, canceller_t * canceller, const char * pubkey) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

    if(timeout_seconds > 0) curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_seconds);

    if(canceller) {
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, canceller_callback);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, canceller);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    }

    if(pubkey) {
        curl_easy_setopt(curl, CURLOPT_PINNEDPUBLICKEY, pubkey);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    }

    std::vector<char> response_data;

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_vector_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

    auto res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if(res != CURLE_OK) {
         fprintf(stderr, "curl_easy_perform() failed: %s\n",
                 curl_easy_strerror(res));
         return {"", -1};
     }

     return {std::string(response_data.data(), response_data.size()), http_code};
}

response_t post(std::string url, std::string payload, const headers_t & headers, int timeout_seconds, canceller_t * canceller) {
    auto curl = curl_easy_init();
    if(curl) {
        curl_slist * headers_list = nullptr;
        std::vector<std::string> headers_vec(headers.size());

        for(auto kv: headers) {
            headers_vec.push_back(kv.first + ": " + kv.second);
        }

        for(auto & header: headers_vec) headers_list = curl_slist_append(headers_list, header.c_str());

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers_list);
        
        auto response = perform_request(curl, url, timeout_seconds, canceller, nullptr);

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers_list);

        return response;
     }
    return {"", -1};
}

void init_remote() {
    curl_global_init(CURL_GLOBAL_ALL);
}

void cleanup_remote() {
    curl_global_cleanup();
}

std::string url_encode(CURL * curl, const std::string & in) {
    auto out = curl_easy_escape(curl, in.c_str(), in.size());
    if(out) {
        std::string result = out;
        curl_free(out);
        return result;
    } else return "";
}

response_t get(std::string url, const params_t & params, int timeout_seconds, canceller_t* canceller, const char* pubkey) {

    auto curl = curl_easy_init();
    if(curl) {
        bool first = true;
        for(auto param: params) {
            char prefix = first ? '?' : '&';
            first = false;
            url += prefix + param.first + '=' + url_encode(curl, param.second);
        }
        auto response = perform_request(curl, url, timeout_seconds, canceller, pubkey);
        curl_easy_cleanup(curl);
        return response;
    }
    return {"", -1};
}
