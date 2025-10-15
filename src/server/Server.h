#ifndef SERVER_H
#define SERVER_H

#include "Discovery.h"
#include "Process.h"
#include "Interface.h"
#include <string>
#include <map>
#include <queue>
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>

#define STARTING_BALANCE 1000

struct ClientData {
    std::string ip; // IP do cliente, usado como chave
    int balance; // Saldo do cliente
    int seq_num; // Último número de sequência registrado do cliente

    ClientData(std::string i, int b, int s): ip(i), balance(b), seq_num(s) {}
};


std::map<std::string, ClientData> clients;
std::mutex mutex_client_list;

std::queue<std::string> clients_to_add;
std::mutex mutex_new_clients;

int total_balance;

void add_clients();


#endif