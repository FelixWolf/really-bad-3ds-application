#ifndef NETWORK_H_
#define NETWORK_H_
#include <3ds.h>
#include <cstddef>
#include <string>
#include <malloc.h>

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000

void netInit();



class Network{
protected:
    enum NetworkState_t {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        DISCONNECTING
    };
    
    NetworkState_t state;
    int sockfd;
    
    struct addrinfo *server_addr = NULL;
    
    std::string error;
    
public:
    Network();
    ~Network();
    bool connect(std::string host, uint16_t port);
    void disconnect();
    
    bool connected(){
        return state == CONNECTED;
    }
    
    bool send(uint8_t *data, uint16_t length);
    int recv(uint8_t *data, uint16_t length);
};

#endif