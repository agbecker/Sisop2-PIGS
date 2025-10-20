#ifndef INTERFACE_H
#define INTERFACE_H

#include <string>
#include <netinet/in.h>

#include <iostream>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <arpa/inet.h>
#include <regex>

#include "../Utils.h"


typedef struct Command{
    struct in_addr dest; // destination IP
    double amount;       // amount to transfer
} Command;

class Interface {
    private:
        Command current_command;
        struct in_addr server_addr; 
        RequestReply executeCommand(Command command);
        
        Command getCommand();
        void printCommandResult(double new_balance, int command_count);

    public: 
        Interface(in_addr server_addr) : server_addr(server_addr) { }
        void run();
    };



#endif