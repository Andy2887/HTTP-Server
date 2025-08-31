#include "http_server.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include <iostream>
#include <string>

// Handler functions
HttpResponse handle_root(const HttpRequest& request) {
    HttpResponse response;
    response.status_code = HttpStatus::OK;
    return response;
}

HttpResponse handle_echo(const HttpRequest& request) {
    HttpResponse response;
    response.status_code = HttpStatus::OK;
    response.headers["Content-Type"] = "text/plain";
    
    // Extract echo string from path /echo/something
    std::string echo_str = request.path.substr(6); // Remove "/echo/"
    response.body = echo_str;
    response.headers["Content-Length"] = std::to_string(echo_str.size());
    
    return response;
}

HttpResponse handle_user_agent(const HttpRequest& request) {
    HttpResponse response;
    response.status_code = HttpStatus::OK;
    response.headers["Content-Type"] = "text/plain";
    
    auto it = request.headers.find("User-Agent");
    if (it != request.headers.end()) {
        response.body = it->second;
        response.headers["Content-Length"] = std::to_string(it->second.size());
    } else {
        response.body = "";
        response.headers["Content-Length"] = "0";
    }
    
    return response;
}

int main(int argc, char** argv) {
    // Flush after every cout / cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    
    std::cout << "Starting HTTP Server...\n";
    
    HttpServer server(4221);
    
    // Register routes
    server.add_route("/", handle_root);
    server.add_route("/echo/*", handle_echo);
    server.add_route("/user-agent", handle_user_agent);
    
    // Start server
    server.start();
    
    return 0;
}