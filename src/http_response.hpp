#pragma once
#include <string>
#include <map>

enum class HttpStatus {
    OK = 200,
    NOT_FOUND = 404,
    INTERNAL_ERROR = 500
};

struct HttpResponse {
    HttpStatus status_code;
    std::map<std::string, std::string> headers;
    std::string body;
    
    std::string to_string() const;
};