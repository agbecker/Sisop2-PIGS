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

std::map<std::string, ClientData> clients;
std::mutex mutex_client_list;

std::queue<std::string> clients_to_add;
std::mutex mutex_new_clients;

int total_balance;

std::queue<Event> events;

void add_clients();


#endif