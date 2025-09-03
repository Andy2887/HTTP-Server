#pragma once
#include "http_request.hpp"
#include "http_response.hpp"
#include <string>
#include <vector>
#include <functional>

enum class WebSocketOpcode : uint8_t {
    CONTINUATION = 0x0,
    TEXT = 0x1,
    BINARY = 0x2,
    CLOSE = 0x8,
    PING = 0x9,
    PONG = 0xA
};

struct WebSocketFrame {
    bool fin;
    WebSocketOpcode opcode;
    bool masked;
    uint64_t payload_length;
    uint32_t mask_key;
    std::vector<uint8_t> payload;
};

class WebSocketConnection {
public:
    using MessageHandler = std::function<void(const std::string&)>;
    using CloseHandler = std::function<void()>;
    
    WebSocketConnection(int client_fd);
    ~WebSocketConnection();
    
    bool send_text(const std::string& message);
    bool send_binary(const std::vector<uint8_t>& data);
    bool send_ping(const std::string& data = "");
    bool send_pong(const std::string& data = "");
    void close(uint16_t code = 1000, const std::string& reason = "");
    
    void set_message_handler(MessageHandler handler) { message_handler = handler; }
    void set_close_handler(CloseHandler handler) { close_handler = handler; }
    
    void handle_messages(); // Main message loop
    
    static bool is_websocket_request(const HttpRequest& request);
    static HttpResponse create_handshake_response(const HttpRequest& request);
    
private:
    int client_fd;
    bool is_connected;
    MessageHandler message_handler;
    CloseHandler close_handler;
    
    bool send_frame(WebSocketOpcode opcode, const std::vector<uint8_t>& payload);
    WebSocketFrame parse_frame(const std::vector<uint8_t>& data, size_t& bytes_consumed);
    std::vector<uint8_t> create_frame(WebSocketOpcode opcode, const std::vector<uint8_t>& payload);
    void handle_frame(const WebSocketFrame& frame);
    static std::string calculate_accept_key(const std::string& websocket_key);
};