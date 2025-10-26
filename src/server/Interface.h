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
#include <mutex>

#include "../Utils.h"

class Interface {
    private:
        void show_stats(bool duplicate, ServerStats stats);
        std::queue<Event> *events;
        std::mutex *mtx_event;
        void display_event(Event event);
    public:
        Interface(std::queue<Event> *e, std::mutex *mtx): events(e), mtx_event(mtx) {};
        void run();
};


#endif