#include "http_server.hpp"
#include "http_parser.hpp"
#include "websocket.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <thread>

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

std::string HttpServer::extract_request(std::string& raw_buffer){
    // Find where the header ends
    size_t header_end = raw_buffer.find("\r\n\r\n");

    size_t cont_len = 0;

    if (header_end != std::string::npos){
        std::string req_line_and_headers = raw_buffer.substr(0, header_end);
        // Find content length
        size_t len_start = req_line_and_headers.find("Content-Length:");
        
        if (len_start != std::string::npos){
            len_start += std::strlen("Content-Length:");
            size_t len_end = req_line_and_headers.find("\r\n", len_start);
            std::string len_str = req_line_and_headers.substr(len_start, len_end - len_start);
            len_str.erase(0, len_str.find_first_not_of(" \t")); // Remove leading spaces
            len_str.erase(len_str.find_last_not_of(" \t") + 1); // Remove trailing spaces
            cont_len = std::stoul(len_str);
        }
    }
    else{
        return "";
    }

    return raw_buffer.substr(0, header_end + cont_len + 4);
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
        
        //
        // Spawn a new thread for each client
        // std::thread: when you create a std::thread, you provide a function and its arguments.
        // The thread starts running that function independently from the main thread.
        // In this case, the arguments are [this, client_fd]. "this" refers to the instance of HttpServer
        // detach(): a method of std::thread that lets the thread run independently in the background.
        // After calling detach(), you cannot join or interact with the thread after detaching. 
        //
        std::thread([this, client_fd](){
            handle_client(client_fd);
        }).detach();
    }
}

void HttpServer::handle_client(int client_fd) {
    std::string raw_buffer;
    char buffer[4096];

    // First loop: keep accepting bytes from client
    while (true){
        ssize_t bytes_received = read(client_fd, buffer, sizeof(buffer) - 1);
        
        if (bytes_received <= 0) {
            // Client closed connection or error
            break;
        }

        buffer[bytes_received] = '\0';
        raw_buffer += buffer;
        // Second loop: keep processing the accepted bytes from client
        while (true){
            // Extract request from buffer
            std::string raw_request = extract_request(raw_buffer);

            if (raw_request.empty()) {
                break;
            }

            // Parse request
            HttpRequest request = HttpParser::parse_request(raw_request);
            
            // Check if this is a WebSocket upgrade request
            if (WebSocketConnection::is_websocket_request(request)) {
                // Send WebSocket handshake response
                HttpResponse response = WebSocketConnection::create_handshake_response(request);
                std::string response_str = HttpParser::serialize_response(response);
                write(client_fd, response_str.c_str(), response_str.size());
                
                // Create WebSocket connection and handle messages
                WebSocketConnection ws_conn(client_fd);
                
                // Set up message handler
                ws_conn.set_message_handler([&ws_conn](const std::string& message) {
                    std::cout << "Received WebSocket message: " << message << std::endl;
                    // Echo the message back
                    ws_conn.send_text("Echo: " + message);
                });
                
                // Set up close handler
                ws_conn.set_close_handler([]() {
                    std::cout << "WebSocket connection closed" << std::endl;
                });
                
                // Handle WebSocket messages (this will block until connection closes)
                ws_conn.handle_messages();
                
                // WebSocket connection closed, exit client handler
                close(client_fd);
                return;
            }
            
            // Route regular HTTP request
            HttpResponse response = router.route(request);
            
            // Send response
            std::string response_str = HttpParser::serialize_response(response);
            ssize_t bytes_sent = write(client_fd, response_str.c_str(), response_str.size());
            
            if (bytes_sent < 0) {
                std::cerr << "Failed to send response\n";
            } else {
                std::cout << "Sent " << bytes_sent << " bytes\n";
            }

            // Remove processed request from raw_buffer
            raw_buffer.erase(0, raw_request.size());

        }
    }

    
    close(client_fd);
}

void HttpServer::add_route(const std::string& path, HttpRouter::Handler handler) {
    router.add_route(path, handler);
}