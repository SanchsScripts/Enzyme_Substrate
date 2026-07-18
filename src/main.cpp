#include <iostream>
#include<bits/stdc++.h>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include<thread>
#include <netdb.h>
using namespace std;
struct HTTPRequest {
    string method;
    string path;
    string version;
    map<string, string> headers;
    string body;
};

// Helper function to parse the raw HTTP string into our struct
HTTPRequest parse_request(const string& request) {
    HTTPRequest req;
    stringstream ss(request);
    string line;

    // 1. Parse the Request Line (e.g., "GET /user-agent HTTP/1.1")
    if (getline(ss, line)) {
        // Remove trailing '\r' if it exists
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        istringstream line_ss(line);
        line_ss >> req.method >> req.path >> req.version;
    }

    // 2. Parse the Headers
    while (getline(ss, line) && line != "\r" && !line.empty()) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        size_t pos = line.find(":");
        if (pos != string::npos) {
            string header_name = line.substr(0, pos);
            // Skip the colon and the space after it
            size_t value_start = pos + 1;
            if (value_start < line.length() && line[value_start] == ' ') {
                value_start++;
            }
            string header_value = line.substr(value_start);
            req.headers[header_name] = header_value;
        }
    }

    return req;
}
void handle_client(int client_fd, string directory) {
    string message(1024, '\0');
    ssize_t bytes_read = recv(client_fd, (void *)&message[0], message.max_size(), 0);
    
    if (bytes_read == -1){
        cerr << "Read failed\n";
        close(client_fd);
        return;
    }
    
    message.resize(bytes_read);
    HTTPRequest request = parse_request(message);
    string response;
    
    // Routing logic
    if (request.path == "/") {
        response = "HTTP/1.1 200 OK\r\n\r\n";
    } else if (request.path.find("/echo/") == 0) {
        string content = request.path.substr(6);
        response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + to_string(content.size()) + "\r\n\r\n" + content;
    } else if (request.path == "/user-agent") {
        string user_agent = request.headers["User-Agent"];
        response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + to_string(user_agent.size()) + "\r\n\r\n" + user_agent;
    } else if (request.path.find("/files/") == 0) {
        // Extract the filename from the path
        string filename = request.path.substr(7);
        
        // Safely construct the full file path
        string filepath = directory;
        if (!filepath.empty() && filepath.back() != '/') {
            filepath += "/";
        }
        filepath += filename;

        // Try to open the file in binary mode
        ifstream file(filepath, ios::binary);
        
        if (file.is_open()) {
            // Read the entire file contents into a stringstream
            stringstream buffer;
            buffer << file.rdbuf();
            string file_contents = buffer.str();
            
            response = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: " + to_string(file_contents.size()) + "\r\n\r\n" + file_contents;
        } else {
            // File does not exist or cannot be opened
            response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        }
    } else {
        response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    }

    send(client_fd, response.c_str(), response.length(), 0);
    close(client_fd); 
}
int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  // TODO: Uncomment the code below to pass the first stage
  //
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  
  std::cout << "Waiting for a client to connect...\n";
  while (true) {
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
        if (client_fd < 0) {
            cerr << "error handling client connection\n";
            continue; // Don't crash the server, just skip to the next potential client
        }
        
        cout << "Client connected\n";
        
        // Spawn a new thread for this client and let it run independently
        thread client_thread(handle_client, client_fd,directory);
        client_thread.detach(); 
  }

  
  close(server_fd);

  return 0;
}
