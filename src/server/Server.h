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

class Server {
    private:
        std::map<std::string, int> clients; // Where the client's IP is the key, and the value is their balance
    
};





#endif