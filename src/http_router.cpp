#include "http_router.hpp"
#include <iostream>

void HttpRouter::add_route(const std::string& path, Handler handler) {
    routes[path] = handler;
}

HttpResponse HttpRouter::route(const HttpRequest& request) {
    // Check for exact path match
    auto it = routes.find(request.path);
    if (it != routes.end()) {
        return it->second(request);
    }
    
    // Check for echo path pattern
    if (request.path.find("/echo/") == 0) {
        auto echo_it = routes.find("/echo/*");
        if (echo_it != routes.end()) {
            return echo_it->second(request);
        }
    }
    
    // Default 404 response
    HttpResponse response;
    response.status_code = HttpStatus::NOT_FOUND;
    return response;
}