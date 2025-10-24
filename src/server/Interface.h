#ifndef INTERFACE_H
#define INTERFACE_H

#include <string>
#include <netinet/in.h>
#include <queue>
#include <iostream>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <arpa/inet.h>
#include <regex>
#include <thread>

#include "../Utils.h"

class Interface {
    private:
        void show_stats();
        int num_transactions;
        long int total_transferred, total_balance;
        std::queue<Event> *events;
    public:
        Interface(std::queue<Event> *e):events(e), num_transactions(0), total_transferred(0), total_balance(0) {};
        void run();
};


#endif