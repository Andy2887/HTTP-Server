#pragma once
#include "http_request.hpp"
#include "http_response.hpp"
#include <string>
#include <vector>
#include <functional>

/*
In the WebSocket protocol (RFC 6455), communication happens through frames. 
A frame is the smallest unit of communication in a WebSocket connection. 
Each frame carries a piece of data (like text, binary, or control info), and 
multiple frames can combine to form a complete WebSocket message.

Each frame has:

1, FIN bit (1 bit) → Tells if this is the final frame of a message.
2, Opcode (4 bits) → Defines the frame type (text, binary, ping, pong, close, continuation).
3, Mask bit (1 bit) → Indicates if the payload data is masked (required for client → server frames).
4, Payload length (7–64 bits) → Size of the actual data.
5, Masking key (0 or 4 bytes) → If masked, the key to unmask the payload.
6, Payload data (variable) → The actual message content.

Note: 
Masking means XOR-ing the payload data with a 4-byte key before sending it.
*/


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
    // Creates a type alias. It represents any callable object (like a function, lambda)
    // that matches the argument and the return type.
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