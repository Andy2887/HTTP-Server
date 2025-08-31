# HTTP Server in C++

A lightweight, modular HTTP/1.1 server implementation built from scratch using TCP sockets in C++. This project demonstrates fundamental networking concepts and HTTP protocol handling without relying on high-level HTTP libraries.

## Architecture

The server is built with a modular design:

```
src/
├── main.cpp              # Entry point and request handlers
├── http_server.hpp/cpp   # Core server functionality and socket management
├── http_parser.hpp/cpp   # HTTP request/response parsing
├── http_router.hpp/cpp   # URL routing and endpoint matching
├── http_request.hpp/cpp  # HTTP request data structure
└── http_response.hpp/cpp # HTTP response data structure and serialization
```

## Features

- **HTTP/1.1 Protocol Support**: Handles standard HTTP requests and responses
- **Persistent Connections**: Supports multiple requests over the same TCP connection
- **RESTful Routing**: Simple routing system for different endpoints
- **WebSocket Protocol Support**: Real-time bidirectional communication for clients and server

## Getting Started

### Prerequisites

- C++23 compatible compiler
- CMake 3.13 or higher
- vcpkg package manager

### Building

1. Clone the repository:

2. Start the project:
```bash
./start.sh
```

Or manually:
```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
cmake --build ./build
```

The server will listen on port 4221 by default.

## API Endpoints

### GET /
Returns a simple 200 OK response.

```bash
curl http://localhost:4221/
```

### GET /echo/{message}
Echoes back the message from the URL path.

```bash
curl http://localhost:4221/echo/hello
# Returns: hello
```

### GET /user-agent
Returns the client's User-Agent header.

```bash
curl http://localhost:4221/user-agent
# Returns: curl/8.7.1
```

## Acknowledgements

This project is implemented following the tutorial from Codecrafters [Build Your Own HTTP Server](https://app.codecrafters.io/courses/http-server/overview). Special thanks to the Codecrafters team for their excellent resources and guidance.