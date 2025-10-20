#ifndef PROCESS_H
#define PROCESS_H

#include "../Utils.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../json.hpp"

#define PORT 5000
#define BUFFER_SIZE 256

class Process {
    private:
        int num_seq;
        RequestReply* rr;
        void sendToServer(std::string request);
        struct in_addr serv_addr;
    public:
        Process(struct in_addr serv, RequestReply* r): num_seq(0), rr(r), serv_addr(serv) {};
        void run();
};



#endif