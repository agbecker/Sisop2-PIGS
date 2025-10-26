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

std::map<std::string, ClientData> clients;
std::mutex mutex_client_list;

std::queue<std::string> clients_to_add;
std::mutex mutex_new_clients;

std::queue<Event> events;
std::mutex mtx_events;

ServerStats stats;

void add_clients();


#endif