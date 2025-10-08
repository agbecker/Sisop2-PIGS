#ifndef SERVER_H
#define SERVER_H

#include "Discovery.h"
#include "Process.h"
#include "Interface.h"
#include <string>
#include <map>
#include <iostream>
#include <thread>
#include <chrono>

#define STARTING_BALANCE 1000

struct ClientData {
    std::string ip; // IP do cliente, usado como chave
    int balance; // Saldo do cliente
    int seq_num; // Último número de sequência registrado do cliente
};

class Server {
    private:
        std::map<std::string, ClientData> clients;
    
};

#endif