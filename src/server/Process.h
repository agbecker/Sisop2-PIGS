#ifndef PROCESS_H
#define PROCESS_H

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

struct ClientData {
    std::string ip; // IP do cliente, usado como chave
    int balance; // Saldo do cliente
    int seq_num; // Último número de sequência registrado do cliente

    ClientData(std::string i, int b, int s): ip(i), balance(b), seq_num(s) {}
};

#endif