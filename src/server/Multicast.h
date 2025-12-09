#ifndef MULTICAST_H
#define MULTICAST_H

#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

class Multicast {
    private:
        int sock;

    public:
        Multicast() = default;
        void init();
};

#endif