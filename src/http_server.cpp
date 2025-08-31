#include "http_server.hpp"
#include "http_parser.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>

HttpServer::HttpServer(int port) : port(port), server_fd(-1) {}

HttpServer::~HttpServer() {
    if (server_fd >= 0) {
        close(server_fd);
    }
}

bool HttpServer::setup_socket() {
    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create server socket\n";
        return false;
    }
    
    // Configure socket
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "setsockopt failed\n";
        return false;
    }
    
    // Bind to port
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        std::cerr << "Failed to bind to port " << port << "\n";
        return false;
    }
    
    // Listen for connections
    if (listen(server_fd, 5) != 0) {
        std::cerr << "listen failed\n";
        return false;
    }
    
    return true;
}

void HttpServer::start() {
    if (!setup_socket()) {
        return;
    }
    
    std::cout << "Server listening on port " << port << "\n";
    
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    while (true) {
        std::cout << "Waiting for a client to connect...\n";
        
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_fd < 0) {
            std::cerr << "Failed to accept client connection\n";
            continue;
        }
        
        std::cout << "Client connected\n";
        handle_client(client_fd);
    }
}

void HttpServer::handle_client(int client_fd) {
    char buffer[4096];
    ssize_t bytes_received = read(client_fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        std::string raw_request(buffer);
        
        // Parse request
        HttpRequest request = HttpParser::parse_request(raw_request);
        
        // Route request
        HttpResponse response = router.route(request);
        
        // Send response
        std::string response_str = HttpParser::serialize_response(response);
        write(client_fd, response_str.c_str(), response_str.size());
    }
    
    close(client_fd);
}

void HttpServer::add_route(const std::string& path, HttpRouter::Handler handler) {
    router.add_route(path, handler);
}