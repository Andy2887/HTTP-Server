#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

using namespace std;

void handle_client(int client_fd){
  const char* response = "HTTP/1.1 200 OK\r\n\r\n";
  ssize_t bytes_sent = write(client_fd, response, strlen(response));
  close(client_fd);
}

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";


  // ========================== STEP 1: Create the Socket ==========================

  // Create a socket and return the id
  // AF_INET: the socket will use TCP
  // SOCK_STREAM: indicate the socket will use a stream protocol
  // 0: protocl number, 0 means use the default protocl
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // ========================== STEP 2: Configure the Socket ==========================

  int reuse = 1;

  //
  // setsockopt: configuring certain behaviors or properties of the socket
  // SOL_SOCKET: specifies that option is at socket level
  // SO_REUSEADDR: allows the socket to bind to an address that is in a TIME_WAIT state
  //
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  // ========================== STEP 3: Bind to a Port ==========================

  //
  // sockarr_in: a struct for setting the address information for binding the socket
  // sin_family specifies the address family (IPv4).
  // sin_addr.s_addr sets the IP address to any local address.
  // sin_port sets the port number
  // htons(): converts a 16-bit integer from host byte order to network byte order.
  //
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }
  
  int connection_backlog = 5;

  // ========================== STEP 4: Listening for Connections ==========================

  //
  // The listen function in C++ is typically used in network programming to mark a socket as a passive socket that will accept incoming connection requests. 
  // It takes two parameters: the socket file descriptor and the backlog, which specifies the maximum number of pending connections the queue can hold.
  // 
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  
  std::cout << "Waiting for a client to connect...\n";
  
  int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
  if (client_fd < 0){
    std::cerr << "Failed to accept client connection\n";
    close(server_fd);
    return 1;
  }

  std::cout << "Client connected\n";
  handle_client(client_fd);
  
  close(server_fd);

  return 0;
}