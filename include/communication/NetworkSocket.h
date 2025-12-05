#ifndef NETWORKSOCKET_H
#define NETWORKSOCKET_H

#include "Communication.h"
#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>  // For close()


template<typename Generic_type>
class NetworkSocket : public Communication<Generic_type> {
private:
    int sock_fd;  // Socket file descriptor
    struct sockaddr_in server_addr;

public:
    explicit NetworkSocket(Generic_type* data = nullptr);
    ~NetworkSocket();
    
    // Send data over the socket
    bool send_data(const Generic_type& data) override;

    // Receive data from the socket
    Generic_type* receive_data() override;

    // Clean up and close the socket
    void clean_up() override;

};


template<typename Generic_type>
NetworkSocket<Generic_type>::NetworkSocket(Generic_type* data)
    : Communication<Generic_type>(data), sock_fd(-1) {

    // Create a socket TCP 
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        throw std::runtime_error("Socket creation failed!");
    }

    // Configure : server is on localhost and port 8080
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);  
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Allow any address

    // Connect to server and assume the server is already running
    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        throw std::runtime_error("Connection to server failed!");
    }
}

// Send data over the socket
template<typename Generic_type>
bool NetworkSocket<Generic_type>::send_data(const Generic_type& data) {
    std::cout << "Sending data via socket: " << data << std::endl;

    ssize_t sent_bytes = send(sock_fd, &data, sizeof(Generic_type), 0);
    return sent_bytes == sizeof(Generic_type);
}

// Receive data from the socket
template<typename Generic_type>
Generic_type* NetworkSocket<Generic_type>::receive_data() {
    std::cout << "Receiving data via socket...\n";

    // Buffer to store received data
    Generic_type* received_data = this->generic_data;

    ssize_t received_bytes = recv(sock_fd, received_data, sizeof(Generic_type), 0);
    if (received_bytes <= 0) {
        std::cerr << "Failed to receive data or connection closed.\n";
        return nullptr;
    }

    return received_data;
}

// Clean up and close the socket
template<typename Generic_type>
void NetworkSocket<Generic_type>::clean_up() {
    std::cout << "Closing socket...\n";
    if (sock_fd != -1) {
        close(sock_fd);
        sock_fd = -1;
    }
}

template<typename Generic_type>
NetworkSocket<Generic_type>::~NetworkSocket() {
    clean_up();  // Ensure cleanup 
}

#endif
