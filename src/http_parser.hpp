#pragma once
#include "http_request.hpp"
#include "http_response.hpp"
#include <string>

class HttpParser {
public:
    static HttpRequest parse_request(const std::string& raw_request);
    static std::string serialize_response(const HttpResponse& response);
private:
    static std::string trim(const std::string& str);
};