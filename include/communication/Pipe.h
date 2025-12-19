#ifndef PIPE_H
#define PIPE_H

#include "Communication.h"
#include <string>
#include <mutex>
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <type_traits>
#include <optional>
#include <iostream>

template<typename Generic_type>
class Pipe : public Communication<Generic_type> {
    static_assert(std::is_trivially_copyable<Generic_type>::value,
                  "Pipe only supports trivially copyable types (int, double, char, etc.)");

private:
    std::string pipe_name;
    int fd = -1;
    std::mutex pipe_mutex;
    std::optional<Generic_type> buffer; // safely store received data

public:
    explicit Pipe(const std::string& name, int IO_type = O_RDWR | O_NONBLOCK);
    ~Pipe();

    Pipe(const Pipe&) = delete;             // disable copy
    Pipe& operator=(const Pipe&) = delete;  // disable copy
    Pipe(Pipe&& other) noexcept;            // enable move
    Pipe& operator=(Pipe&& other) noexcept; // enable move

    bool send_data(const Generic_type& data) override;
    Generic_type* receive_data() override;
    void clean_up() override;
    int get_fd() const { return fd; }
    std::string get_name() const { return pipe_name; }
};

// Constructor: create/open named pipe
template<typename Generic_type>
Pipe<Generic_type>::Pipe(const std::string& name, int IO_type)
    : Communication<Generic_type>(nullptr), pipe_name(name) {

    if (access(pipe_name.c_str(), F_OK) == -1) {
        if (mkfifo(pipe_name.c_str(), 0666) == -1) {
            throw std::runtime_error("mkfifo failed for: " + pipe_name);
        }
    }

    fd = open(pipe_name.c_str(), IO_type);
    if (fd == -1) {
        throw std::runtime_error("Failed to open pipe: " + pipe_name);
    }
}

// Destructor: close pipe
template<typename Generic_type>
Pipe<Generic_type>::~Pipe() {
    if (fd != -1) close(fd);
}

// Move constructor
template<typename Generic_type>
Pipe<Generic_type>::Pipe(Pipe&& other) noexcept
    : pipe_name(std::move(other.pipe_name)), fd(other.fd) {
    other.fd = -1;
}

// Move assignment
template<typename Generic_type>
Pipe<Generic_type>& Pipe<Generic_type>::operator=(Pipe&& other) noexcept {
    if (this != &other) {
        if (fd != -1) close(fd);
        pipe_name = std::move(other.pipe_name);
        fd = other.fd;
        other.fd = -1;
    }
    return *this;
}

// Send data through the pipe
template<typename Generic_type>
bool Pipe<Generic_type>::send_data(const Generic_type& data) {
    std::lock_guard<std::mutex> lock(pipe_mutex);
    if (fd == -1) return false;

    ssize_t bytes = write(fd, &data, sizeof(Generic_type));
    return bytes == sizeof(Generic_type);
}

// Receive data from the pipe
template<typename Generic_type>
Generic_type* Pipe<Generic_type>::receive_data() {
    std::lock_guard<std::mutex> lock(pipe_mutex);
    if (fd == -1) return nullptr;

    Generic_type tmp;
    ssize_t bytes = read(fd, &tmp, sizeof(Generic_type));
    if (bytes == sizeof(Generic_type)) {
        buffer = tmp;
        return &(*buffer);
    }
    return nullptr;
}

// Clean up the pipe (unlink the FIFO file)
template<typename Generic_type>
void Pipe<Generic_type>::clean_up() {
    std::lock_guard<std::mutex> lock(pipe_mutex);
    if (!pipe_name.empty()) {
        unlink(pipe_name.c_str());
    }
}

#endif // PIPE_H
