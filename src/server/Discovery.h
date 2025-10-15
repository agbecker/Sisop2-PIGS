#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <iostream>
#include <chrono>
#include <thread>
#include <queue>
#include <mutex>

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
        void treat_request(std::string message, struct sockaddr_in cli_addr);
        int sockfd; // ID do socket
        char buf[BUFFER_SIZE]; // Buffer para mensagens

    public:
        void set_str(std::string* str);
        void run();
        void awaitRequest();
        std::queue<std::string>* clients_to_add;
        std::mutex* mutex_add_client;
};



#endif