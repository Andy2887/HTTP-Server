#include "websocket.hpp"
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/socket.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <algorithm>
#include <cstring>

const std::string WEBSOCKET_MAGIC_STRING = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

WebSocketConnection::WebSocketConnection(int client_fd) 
    : client_fd(client_fd), is_connected(true) {}

WebSocketConnection::~WebSocketConnection() {
    if (is_connected) {
        close();
    }
}

// Check if the request is a websocket request
bool WebSocketConnection::is_websocket_request(const HttpRequest& request) {
    auto connection_it = request.headers.find("Connection");
    auto upgrade_it = request.headers.find("Upgrade");
    auto version_it = request.headers.find("Sec-WebSocket-Version");
    auto key_it = request.headers.find("Sec-WebSocket-Key");
    
    return connection_it != request.headers.end() &&
           upgrade_it != request.headers.end() &&
           version_it != request.headers.end() &&
           key_it != request.headers.end() &&
           connection_it->second.find("Upgrade") != std::string::npos &&
           upgrade_it->second == "websocket" &&
           version_it->second == "13";
}

// Generate the value for Sec-WebSocket-Accept header required by WebSocket handshake response
std::string WebSocketConnection::calculate_accept_key(const std::string& websocket_key) {
    std::string combined = websocket_key + WEBSOCKET_MAGIC_STRING;
    
    /*
        SHA-1 hashing
    */
    // The combined string is hashed using SHA-1
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(combined.c_str()), combined.length(), hash);
    
    /*
        Base64 encoding
    */
    // Creates a new memory BIO (buffered I/O object) for storing data in memory
    BIO* bio = BIO_new(BIO_s_mem());
    // Creates a new BIO filter for Base64 encoding
    BIO* b64 = BIO_new(BIO_f_base64());
    // Sets the Base64 BIO to not insert newlines in the output
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    // Chains the Base64 filter BIO on top of the memory BIO
    bio = BIO_push(b64, bio);

    // Writes the SHA-1 hash to the BIO chain, which encodes it as Base64
    BIO_write(bio, hash, SHA_DIGEST_LENGTH);
    // Flushes the BIO to ensure all data is processed and written
    BIO_flush(bio);
    
    // Retrieves a pointer to the memory buffer containing the Base64-encoded data
    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);
    
    // Constructs a std::string from the Base64-encoded data
    std::string result(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);
    
    return result;
}

HttpResponse WebSocketConnection::create_handshake_response(const HttpRequest& request) {
    HttpResponse response;
    response.status_code = static_cast<HttpStatus>(101); // Switching Protocols
    
    auto key_it = request.headers.find("Sec-WebSocket-Key");
    if (key_it != request.headers.end()) {
        std::string accept_key = calculate_accept_key(key_it->second);
        response.headers["Upgrade"] = "websocket";
        response.headers["Connection"] = "Upgrade";
        response.headers["Sec-WebSocket-Accept"] = accept_key;
    }
    
    return response;
}

bool WebSocketConnection::send_text(const std::string& message) {
    std::vector<uint8_t> payload(message.begin(), message.end());
    return send_frame(WebSocketOpcode::TEXT, payload);
}

bool WebSocketConnection::send_binary(const std::vector<uint8_t>& data) {
    return send_frame(WebSocketOpcode::BINARY, data);
}

bool WebSocketConnection::send_ping(const std::string& data) {
    std::vector<uint8_t> payload(data.begin(), data.end());
    return send_frame(WebSocketOpcode::PING, payload);
}

bool WebSocketConnection::send_pong(const std::string& data) {
    std::vector<uint8_t> payload(data.begin(), data.end());
    return send_frame(WebSocketOpcode::PONG, payload);
}

void WebSocketConnection::close(uint16_t code, const std::string& reason) {
    if (!is_connected) return;
    
    std::vector<uint8_t> payload;
    payload.push_back((code >> 8) & 0xFF);
    payload.push_back(code & 0xFF);
    payload.insert(payload.end(), reason.begin(), reason.end());
    
    send_frame(WebSocketOpcode::CLOSE, payload);
    is_connected = false;
    ::close(client_fd);
}

std::vector<uint8_t> WebSocketConnection::create_frame(WebSocketOpcode opcode, const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> frame;
    
    // First byte: FIN (1) + RSV (000) + Opcode (4 bits)
    frame.push_back(0x80 | static_cast<uint8_t>(opcode));
    
    // Payload length
    if (payload.size() < 126) {
        frame.push_back(static_cast<uint8_t>(payload.size()));
    } else if (payload.size() < 65536) {
        frame.push_back(126);
        frame.push_back((payload.size() >> 8) & 0xFF);
        frame.push_back(payload.size() & 0xFF);
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; i--) {
            frame.push_back((payload.size() >> (i * 8)) & 0xFF);
        }
    }
    
    // Payload
    frame.insert(frame.end(), payload.begin(), payload.end());
    
    return frame;
}

bool WebSocketConnection::send_frame(WebSocketOpcode opcode, const std::vector<uint8_t>& payload) {
    if (!is_connected) return false;
    
    std::vector<uint8_t> frame = create_frame(opcode, payload);
    
    ssize_t bytes_sent = send(client_fd, frame.data(), frame.size(), 0);
    return bytes_sent == static_cast<ssize_t>(frame.size());
}

WebSocketFrame WebSocketConnection::parse_frame(const std::vector<uint8_t>& data, size_t& bytes_consumed) {
    WebSocketFrame frame{};
    bytes_consumed = 0;
    
    if (data.size() < 2) return frame; // Not enough data
    
    // Parse first byte
    frame.fin = (data[0] & 0x80) != 0;
    frame.opcode = static_cast<WebSocketOpcode>(data[0] & 0x0F);
    
    // Parse second byte
    frame.masked = (data[1] & 0x80) != 0;
    uint64_t payload_len = data[1] & 0x7F;
    
    size_t header_size = 2;
    
    // Extended payload length
    if (payload_len == 126) {
        if (data.size() < 4) return frame;
        payload_len = (data[2] << 8) | data[3];
        header_size += 2;
    } else if (payload_len == 127) {
        if (data.size() < 10) return frame;
        payload_len = 0;
        for (int i = 0; i < 8; i++) {
            payload_len = (payload_len << 8) | data[2 + i];
        }
        header_size += 8;
    }
    
    frame.payload_length = payload_len;
    
    // Mask key
    if (frame.masked) {
        if (data.size() < header_size + 4) return frame;
        frame.mask_key = (data[header_size] << 24) |
                        (data[header_size + 1] << 16) |
                        (data[header_size + 2] << 8) |
                        data[header_size + 3];
        header_size += 4;
    }
    
    // Check if we have complete frame
    if (data.size() < header_size + payload_len) return frame;
    
    // Extract payload
    frame.payload.resize(payload_len);
    for (uint64_t i = 0; i < payload_len; i++) {
        uint8_t byte = data[header_size + i];
        if (frame.masked) {
            byte ^= ((frame.mask_key >> (24 - (i % 4) * 8)) & 0xFF);
        }
        frame.payload[i] = byte;
    }
    
    bytes_consumed = header_size + payload_len;
    return frame;
}

void WebSocketConnection::handle_frame(const WebSocketFrame& frame) {
    switch (frame.opcode) {
        case WebSocketOpcode::TEXT:
            if (message_handler) {
                std::string message(frame.payload.begin(), frame.payload.end());
                message_handler(message);
            }
            break;
            
        case WebSocketOpcode::BINARY:
            // Handle binary data if needed
            break;
            
        case WebSocketOpcode::PING:
            // Respond with pong
            send_pong(std::string(frame.payload.begin(), frame.payload.end()));
            break;
            
        case WebSocketOpcode::PONG:
            // Handle pong response
            break;
            
        case WebSocketOpcode::CLOSE:
            if (close_handler) {
                close_handler();
            }
            close();
            break;
            
        default:
            break;
    }
}

void WebSocketConnection::handle_messages() {
    std::vector<uint8_t> buffer;
    char read_buffer[4096];
    
    while (is_connected) {
        ssize_t bytes_received = recv(client_fd, read_buffer, sizeof(read_buffer), 0);
        
        if (bytes_received <= 0) {
            // Connection closed or error
            is_connected = false;
            break;
        }
        
        // Append to buffer
        buffer.insert(buffer.end(), read_buffer, read_buffer + bytes_received);
        
        // Process frames
        while (buffer.size() >= 2) {
            size_t bytes_consumed = 0;
            WebSocketFrame frame = parse_frame(buffer, bytes_consumed);
            
            if (bytes_consumed == 0) {
                // Incomplete frame, wait for more data
                break;
            }
            
            handle_frame(frame);
            
            // Remove processed bytes
            buffer.erase(buffer.begin(), buffer.begin() + bytes_consumed);
        }
    }
}