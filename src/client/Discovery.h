#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <iostream>

// Bibliotecas para conexão
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BROADCAST_PORT 4000
#define BUFFER_SIZE 256

void discoverServer();


#endif