#ifndef CLIENT_H
#define CLIENT_H

#include "Discovery.h"
#include "Process.h"
#include "Interface.h"
#include <thread>
#include <sys/socket.h>
#include "../Utils.h"

struct sockaddr_in getOwnIP();


#endif