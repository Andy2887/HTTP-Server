#pragma once
#include "http_router.hpp"
#include <string>

class HttpServer {
public:
    HttpServer(int port);
    ~HttpServer();
    void start();
    void add_route(const std::string& path, HttpRouter::Handler handler);
    
private:
    void handle_client(int client_fd);
    int server_fd;
    int port;
    HttpRouter router;
    bool setup_socket();
};