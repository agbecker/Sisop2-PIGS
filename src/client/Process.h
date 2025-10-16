#ifndef PROCESS_H
#define PROCESS_H

#include "../json.hpp"
using json = nlohmann::json;


#include <iostream>

#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define REQUEST_PORT 4001
#define BUFFER_SIZE 256


#endif