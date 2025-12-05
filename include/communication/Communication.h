// include/communication/Communication.h
#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <string>
#include <mutex>

template<typename Generic_type>
class Communication {
protected:
    enum class CommunicationType { SharedMemoryData, Pipe, NetworkSocket };

    Generic_type* generic_data = nullptr;

public:
    explicit Communication(Generic_type* data = nullptr);
    virtual ~Communication() = default;

    virtual bool send_data(const Generic_type& data) = 0;
    virtual Generic_type* receive_data() = 0;
    virtual void clean_up() = 0;
    virtual void update(double dt) {}
};

// Constructor definition
template<typename Generic_type>
Communication<Generic_type>::Communication(Generic_type* data) : generic_data(data) {}


#endif // COMMUNICATION_H