// include/communication/SharedMemoryData.h
#ifndef SHAREDMEMORYDATA_H
#define SHAREDMEMORYDATA_H

#include "Communication.h"
#include <string>
#include <stdexcept>
#include <sys/mman.h>  // mmap, munmap
#include <fcntl.h>     // shm_open
#include <unistd.h>    // close, ftruncate
#include <cstring>     // strerror
#include <cerrno>      // errno
#include <semaphore.h> // POSIX semaphores
#include "config.h"   // SHM_NAME

template<typename Generic_type>
class SharedMemoryData : public Communication<Generic_type> {
private:
    int shm_fd = -1;               // Shared memory file descriptor
    std::string shm_name;          // Shared memory name
    void* mapped_ptr = nullptr;    // Pointer to mapped shared memory
    sem_t* semaphore = nullptr;    // POSIX semaphore for cross-process/thread sync

public:
    explicit SharedMemoryData(const std::string& name = SHM_NAME);
    ~SharedMemoryData() override;

    // Locking mechanism by using a lambda function
    template<typename Func>
    void with_lock(Func f);

    //not thread safe 
    bool send_data(const Generic_type& data) override;
    Generic_type* receive_data() override;
    //not thread safe 
    
    void clean_up() override;
    void update(double dt) override;
};

template<typename Generic_type>
SharedMemoryData<Generic_type>::SharedMemoryData(const std::string& name)
    : Communication<Generic_type>(nullptr), shm_name(name) {
        
    // Open or create the shared memory object
    shm_fd = shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        throw std::runtime_error("shm_open failed: " + std::string(strerror(errno)));
    }

    // Resize shared memory to fit Generic_type
    if (ftruncate(shm_fd, sizeof(Generic_type)) == -1) {
        close(shm_fd);
        throw std::runtime_error("ftruncate failed: " + std::string(strerror(errno)));
    }

    // Map shared memory into address space
    mapped_ptr = mmap(nullptr, sizeof(Generic_type),
                      PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (mapped_ptr == MAP_FAILED) {
        close(shm_fd);
        throw std::runtime_error("mmap failed: " + std::string(strerror(errno)));
    }

    // Assign base class pointer to mapped memory
    this->generic_data = static_cast<Generic_type*>(mapped_ptr);

    // Open or create semaphore (leading "/" required)
    std::string sem_name = BLACKBOARD_SHM_SEM;
    semaphore = sem_open(sem_name.c_str(), O_CREAT, 0666, 1); // binary semaphore
    if (semaphore == SEM_FAILED) {
        munmap(mapped_ptr, sizeof(Generic_type));
        close(shm_fd);
        throw std::runtime_error("sem_open failed: " + std::string(strerror(errno)));
    }
}

template<typename Generic_type>
SharedMemoryData<Generic_type>::~SharedMemoryData() {
    // Unmap shared memory
    if (mapped_ptr != nullptr && mapped_ptr != MAP_FAILED) {
        munmap(mapped_ptr, sizeof(Generic_type));
        mapped_ptr = nullptr;
    }

    // Close shared memory file descriptor
    if (shm_fd != -1) {
        close(shm_fd);
        shm_fd = -1;
    }

    // Close semaphore
    if (semaphore != nullptr) {
        sem_close(semaphore);
        semaphore = nullptr;
    }
}

template<typename Generic_type>
template<typename Func>
void SharedMemoryData<Generic_type>::with_lock(Func f) {
    if (!semaphore || !this->generic_data)
        throw std::runtime_error("Shared memory not initialized");

    sem_wait(semaphore); // lock
    try {
        f(this->generic_data); // run lambda 
    } 
    catch (const std::exception& e) { 
        sem_post(semaphore); 
        throw std::runtime_error(std::string("Exception in shared memory lambda: ") + e.what());
    }
    catch (...) { // catch all other ones
        sem_post(semaphore); // have to release semaphore
        throw std::runtime_error("Something went wrong in shared memory"); 
    }
    sem_post(semaphore); // unlock if no exception occurred
}

//not thread safe 
//

template<typename Generic_type>
bool SharedMemoryData<Generic_type>::send_data(const Generic_type& data) {
    if (!this->generic_data || !semaphore) return false;

    // Lock semaphore
    if (sem_wait(semaphore) < 0) return false;

    // Copy data to shared memory
    std::memcpy(this->generic_data, &data, sizeof(Generic_type));

    // Unlock semaphore
    sem_post(semaphore);
    return true;
}

template<typename Generic_type>
Generic_type* SharedMemoryData<Generic_type>::receive_data() {
    if (!this->generic_data || !semaphore) return nullptr;

    // Lock semaphore
    if (sem_wait(semaphore) < 0) return nullptr;

    // Read pointer to shared memory
    Generic_type* data_ptr = this->generic_data;

    // Unlock semaphore
    sem_post(semaphore);
    return data_ptr;
}

//not thread safe 

template<typename Generic_type>
void SharedMemoryData<Generic_type>::clean_up() {
    // Unlink shared memory
    shm_unlink(shm_name.c_str());

    // Unlink semaphore
    std::string sem_name = BLACKBOARD_SHM_SEM;
    sem_unlink(sem_name.c_str());
}

template<typename Generic_type>
void SharedMemoryData<Generic_type>::update(double dt) {
    // Optional: update a timestamp or version number in shared memory
    // Example: this->generic_data->timestamp += dt;
}

#endif // SHAREDMEMORYDATA_H
