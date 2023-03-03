#include <cstddef>
#include <string>
#include <cstring>
#include <vector>
#include <stdint.h>
#include "network.h"
#include "common.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

static u32 *SOC_buffer;
bool NETWORK_INITIALIZED = false;
void netInit(){
    if(NETWORK_INITIALIZED == true)
        return;
    
    // allocate buffer for SOC service
    SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);

    if(SOC_buffer == NULL) {
        failExit("memalign: failed to allocate\n");
    }

    // Now intialise soc:u service
    int ret;
    if ((ret = socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0) {
        failExit("socInit: 0x%08X\n", (unsigned int)ret);
    }
    NETWORK_INITIALIZED = true;
}

Network::Network(){
    netInit();
}

Network::~Network(){
    
}

bool Network::connect(std::string host, uint16_t port){
    state = CONNECTING;
    
    struct addrinfo hints;

    printf("[NET] Resolving hostname...\n");
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;
    
    int rv;
    struct addrinfo *resaddr = NULL;
    if((rv = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &resaddr)) != 0)
    {
        error = std::string("getaddrinfo() failed: ");
        error.append(gai_strerror(rv));
        error.append(std::to_string(rv).c_str());
        printf("[NET] %s\n", error.c_str());
        state = DISCONNECTED;
        return false;
    }
    
    printf("[NET] Creating socket...\n");
    server_addr = NULL;
    for(struct addrinfo *resaddr_cur = resaddr; resaddr_cur != NULL; resaddr_cur = resaddr_cur->ai_next)
    {
        if((sockfd = socket(resaddr_cur->ai_family, resaddr_cur->ai_socktype, resaddr_cur->ai_protocol)) < 0)
        {
            continue;
        }
        else
        {
            server_addr = resaddr;
            break;
        }
    }
    
    if(server_addr == NULL)
    {
        error = std::string("Failed to create the socket.");
        printf("[NET] %s\n", error.c_str());
        state = DISCONNECTED;
    }
    
    printf("[NET] Configuring socket...\n");
    if(fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK) == -1)
    {
        error = std::string("Failed to set to non-blocking.");
        printf("[NET] %s\n", error.c_str());
        closesocket(sockfd);
        state = DISCONNECTED;
        return false;
    }
    
    printf("[NET] Ready!\n");
    
    state = CONNECTED;
    return true;
}
    
void Network::disconnect(){
    sockfd = 0;
    state = DISCONNECTED;
}

bool Network::send(uint8_t *data, uint16_t length){
    int len = sendto(sockfd, data, length, 0, server_addr->ai_addr, server_addr->ai_addrlen);
    return len != 0;
}

int Network::recv(uint8_t *data, uint16_t length){
    return recvfrom(sockfd,data, length, 0, NULL, NULL);
}