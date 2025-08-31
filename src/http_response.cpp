#include "http_response.hpp"
#include <sstream>

std::string HttpResponse::to_string() const {
    std::ostringstream response;
    
    // Status line
    response << "HTTP/1.1 " << static_cast<int>(status_code);
    switch (status_code) {
        case HttpStatus::OK:
            response << " OK";
            break;
        case HttpStatus::NOT_FOUND:
            response << " Not Found";
            break;
        case HttpStatus::INTERNAL_ERROR:
            response << " Internal Server Error";
            break;
    }
    response << "\r\n";
    
    // Headers
    for (const auto& header : headers) {
        response << header.first << ": " << header.second << "\r\n";
    }
    response << "\r\n";
    
    // Body
    response << body;
    
    return response.str();
}