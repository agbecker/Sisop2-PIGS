#ifndef MULTICAST_H
#define MULTICAST_H

#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#define MCAST_IP "239.0.0.1"
#define MC_DISCOVERY_ASK "ANYONE HERE OINK"

class Multicast {
    private:
        int sock;

    public:
        Multicast() = default;
        void init();
        void find_others(bool* is_only_server);
};

#endif