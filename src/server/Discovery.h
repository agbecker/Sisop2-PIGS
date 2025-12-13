#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <iostream>
#include <chrono>
#include <thread>
#include <queue>
#include <mutex>
#include <vector>

// Bibliotecas para conexão
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../Utils.h"

#define BROADCAST_PORT 4000
#define BUFFER_SIZE 256

class Discovery {
    private:
        std::string *str_pointer;
        void treat_request(std::string message, struct sockaddr_in cli_addr);
        int sockfd; // ID do socket
        char buf[BUFFER_SIZE]; // Buffer para mensagens

    public:
        Discovery(std::queue<std::string>& cli_queue, std::mutex& mtx): clients_to_add(&cli_queue), mutex_add_client(&mtx) {};
        void set_str(std::string* str);
        void run();
        void awaitRequest();
        std::queue<std::string>* clients_to_add;
        std::mutex* mutex_add_client;
        void update_clients_about_main(const std::vector<std::string> ips);
};

static void notify_client_new_server(const std::string& ip);

#endif