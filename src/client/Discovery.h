#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <iostream>
#include "../Utils.h"

// Bibliotecas para conexão
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BROADCAST_PORT 4000
#define NOTIFICATION_PORT 4001
#define BUFFER_SIZE 256

bool discoverServer(struct sockaddr_in &serv_addr);
void awaitNewServer(in_addr* serv_addr);

#endif