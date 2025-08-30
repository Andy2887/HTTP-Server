#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>
#include <vector>

using namespace std;


string handle_root_path(){
  return "HTTP/1.1 200 OK\r\n\r\n";
}

string handle_invalid_path(){
  return "HTTP/1.1 404 Not Found\r\n\r\n";
}

string handle_echo_command(const string& path){
  string echo_str = path.substr(6);
  string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " 
    + to_string(echo_str.size()) + "\r\n\r\n" + echo_str;
  return response;
}

string handle_user_agent_command(vector<string>& headers){
  string user_agent;
  
  // Find the User-Agent header
  for (const string& header : headers) {
    if (header.find("User-Agent:") == 0) {
      user_agent = header.substr(11); // 11 is length of "User-Agent:"
      // Remove leading and trailing spaces and \r
      user_agent.erase(0, user_agent.find_first_not_of(" \t"));
      if (!user_agent.empty() && user_agent.back() == '\r') {
        user_agent.pop_back();
      }
      break;
    }
  }
  
  cout << "User agent: " << user_agent << endl;
  string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " 
    + to_string(user_agent.size()) + "\r\n\r\n" + user_agent;
  return response;
}

void handle_client(int client_fd){
  char buffer[4096];
  ssize_t bytes_received = read(client_fd, buffer, sizeof(buffer) - 1);
  if (bytes_received > 0) {
    buffer[bytes_received] = '\0';
    string request(buffer);
    istringstream request_stream(request);
    string request_line;

    // Read request line
    getline(request_stream, request_line);
    istringstream line_stream(request_line);
    string method, path, version;
    line_stream >> method >> path >> version;
    
    // Read header
    vector<string> headers;
    while (getline(request_stream, request_line)) {
        if (request_line == "\r" || request_line == "") {
            break; // End of headers
        }
        headers.push_back(request_line);
    }

    string response;
    if (path == "/"){
      response = handle_root_path();
    }
    else if (path.find("/echo/") == 0){
      response = handle_echo_command(path);
    }
    else if (path == "/user-agent"){
      response = handle_user_agent_command(headers);
    }
    else {
      response = handle_invalid_path();
    }

    write(client_fd, response.c_str(), response.size());
  }
  close(client_fd);
}

int main(int argc, char **argv) {
  // Flush after every cout / cerr
  cout << unitbuf;
  cerr << unitbuf;
  
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  cout << "Logs from your program will appear here!\n";


  // ========================== STEP 1: Create the Socket ==========================

  // Create a socket and return the id
  // AF_INET: the socket will use TCP
  // SOCK_STREAM: indicate the socket will use a stream protocol
  // 0: protocl number, 0 means use the default protocl
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   cerr << "Failed to create server socket\n";
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
    cerr << "setsockopt failed\n";
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
    cerr << "Failed to bind to port 4221\n";
    return 1;
  }
  
  int connection_backlog = 5;

  // ========================== STEP 4: Listening for Connections ==========================

  //
  // The listen function in C++ is typically used in network programming to mark a socket as a passive socket that will accept incoming connection requests. 
  // It takes two parameters: the socket file descriptor and the backlog, which specifies the maximum number of pending connections the queue can hold.
  // 
  if (listen(server_fd, connection_backlog) != 0) {
    cerr << "listen failed\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  
  cout << "Waiting for a client to connect...\n";
  
  int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
  if (client_fd < 0){
    cerr << "Failed to accept client connection\n";
    close(server_fd);
    return 1;
  }

  cout << "Client connected\n";
  handle_client(client_fd);
  
  close(server_fd);

  return 0;
}