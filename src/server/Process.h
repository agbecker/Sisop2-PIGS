#ifndef PROCESS_H
#define PROCESS_H

#include <iostream>
#include <chrono>
#include <thread>
#include <queue>
#include <mutex>

#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../json.hpp"
#include "../Utils.h"
#include "Multicast.h"

#define BUFFER_SIZE 256

struct ClientData {
    std::string ip; // IP do cliente, usado como chave
    int balance; // Saldo do cliente
    int seq_num; // Último número de sequência registrado do cliente

    ClientData(std::string i, int b, int s): ip(i), balance(b), seq_num(s) {};
    ClientData(): ip("0.0.0.0"), balance(0), seq_num(-1) {};
};

class Process {
    private:
        std::map<std::string, ClientData> *clients; // Ponteiro para a lista de clientes
        int sockfd; // ID do socket
        std::mutex* mtx_clients; // Mutex para acesso à lista de clientes
        void processTransaction(std::string message, struct sockaddr_in cli_addr);
        void sendReply(struct sockaddr_in cli_addr, int status, int new_balance, int seq_num);
        std::queue<Event> *events;
        std::mutex *mtx_events;
        ServerStats *stats;
        int port; // Porta para conexão UDP
        Multicast* multicast;
    public:
        Process (int p, std::map<std::string, ClientData> *c, std::mutex* mtx, std::queue<Event> *e, std::mutex *mtxe, ServerStats *st, Multicast *m): port(p), clients(c), mtx_clients(mtx), events(e), mtx_events(mtxe), stats(st), multicast(m) {};
        void run();

};

std::string serialize_clients(const std::map<std::string, ClientData>* clients);

#endif