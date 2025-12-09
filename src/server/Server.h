#ifndef SERVER_H
#define SERVER_H

#include "Discovery.h"
#include "Process.h"
#include "Interface.h"
#include "Multicast.h"
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

std::fstream transaction_history;

int port;
int id;
bool is_replica_manager;

void add_clients();
void initializeLogFile(std::fstream& handler, const std::string& logPath);

void main_manager();
void main_backup();

#endif