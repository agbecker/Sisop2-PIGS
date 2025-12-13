#ifndef PROCESS_H
#define PROCESS_H

#include "../Utils.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../json.hpp"
#include <iostream>
#include <unistd.h>
#include <errno.h>

#define BUFFER_SIZE 256

class Process {
    private:
        int num_seq;
        RequestReply* rr;
        std::string sendToServer(std::string request);
        struct in_addr *serv_addr;
        int port;
    public:
        Process(int p, struct in_addr* serv, RequestReply* r):port(p), num_seq(1), rr(r), serv_addr(serv) {};
        void run();
};



#endif