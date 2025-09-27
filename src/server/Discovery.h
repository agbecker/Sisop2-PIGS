#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <iostream>
#include <chrono>
#include <thread>

// Bibliotecas para conexão
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BROADCAST_PORT 4000
#define BUFFER_SIZE 256

class Discovery {
    private:
        std::string *str_pointer;

    public:
        void set_str(std::string* str);
        void run();
        void awaitRequest();
};



#endif