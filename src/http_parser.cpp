#include "http_parser.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

HttpRequest HttpParser::parse_request(const std::string& raw_request) {
    HttpRequest request;
    std::istringstream request_stream(raw_request);
    std::string line;
    
    // Parse request line
    if (std::getline(request_stream, line)) {
        std::istringstream line_stream(line);
        line_stream >> request.method >> request.path >> request.version;
    }
    
    // Parse headers
    while (std::getline(request_stream, line) && line != "\r" && !line.empty()) {
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = trim(line.substr(0, colon_pos));
            std::string value = trim(line.substr(colon_pos + 1));
            // Remove trailing \r if present
            if (!value.empty() && value.back() == '\r') {
                value.pop_back();
            }
            request.headers[key] = value;
        }
    }
    
    // Parse body (if any)
    std::string body_line;
    while (std::getline(request_stream, body_line)) {
        request.body += body_line + "\n";
    }
    
    return request;
}

std::string HttpParser::serialize_response(const HttpResponse& response) {
    return response.to_string();
}

std::string HttpParser::trim(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(), 
                                  [](unsigned char ch) { return std::isspace(ch); });
    auto end = std::find_if_not(str.rbegin(), str.rend(), 
                                [](unsigned char ch) { return std::isspace(ch); }).base();
    return (start < end) ? std::string(start, end) : std::string();
}