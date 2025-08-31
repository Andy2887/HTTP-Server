#pragma once
#include "http_request.hpp"
#include "http_response.hpp"
#include <string>
#include <map>
#include <functional>

class HttpRouter {
public:
    using Handler = std::function<HttpResponse(const HttpRequest&)>;
    
    void add_route(const std::string& path, Handler handler);
    HttpResponse route(const HttpRequest& request);
    
private:
    std::map<std::string, Handler> routes;
};