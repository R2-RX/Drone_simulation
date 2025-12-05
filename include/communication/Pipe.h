#ifndef PIPE_H
#define PIPE_H
// =============================================== support trivial types, remember  if you want to pipe other data  ===============================
//                                                          types you need serialization and deserialization
//                                                                like : LV [length][characters...]
//                                                       [length][element1][element2][element3][element4][element5]

//                                                               TLV  [type][length][value]
//                                                       example:       [type = 0x02]          ← tells receiver this is a vector<int>
//                                                                      [length = 5]           ← count of elements
//                                                                      [10][20][30][40][50]   ← 5 integers               

#include "Communication.h"
#include <string>
#include <mutex>
#include <stdexcept>
#include <unistd.h>    // For access(), unlink(), read(), write(), close()
#include <fcntl.h>     // For open()
#include <sys/stat.h>  // For mkfifo
#include <type_traits>
#include <iostream>

template<typename Generic_type>
class Pipe : public Communication<Generic_type> {
    static_assert(std::is_trivially_copyable<Generic_type>::value, 
                  "Pipe only supports trivially copyable types (int, double, char, etc.)");

private:
    std::string pipe_name;
    int fd = -1;
    std::mutex pipe_mutex;

public:
    explicit Pipe(const std::string& name, int IO_type = O_RDWR | O_NONBLOCK);
    ~Pipe();

    bool send_data(const Generic_type& data) override;
    Generic_type* receive_data() override;
    void clean_up() override;
};

// Constructor: create/open named pipe
template<typename Generic_type>
Pipe<Generic_type>::Pipe(const std::string& name, int IO_type)
    : Communication<Generic_type>(nullptr), pipe_name(name) {

    // Create the FIFO if it doesn't exist
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

// Destructor: close pipe and free memory
template<typename Generic_type>
Pipe<Generic_type>::~Pipe() {
    if (fd != -1) close(fd);
    delete this->generic_data;
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

    if (!this->generic_data)
        this->generic_data = new Generic_type();

    ssize_t bytes = read(fd, this->generic_data, sizeof(Generic_type));
    if (bytes == sizeof(Generic_type))
        return this->generic_data;
    else 
        return nullptr;
        throw std::runtime_error("Failed to receive: " + pipe_name);
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
