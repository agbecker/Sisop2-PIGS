#ifndef MULTICAST_H
#define MULTICAST_H

#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>

#define MCAST_IP "239.0.0.1"

#define HEARTBEAT "OINK OINK"
#define HEARTBEAT_PERIOD 100

class Multicast {
    private:
        int sock;
        sockaddr_in group;

    public:
        Multicast() = default;
        void init();
        void find_others(bool* is_only_server);
        void welcome_new_replicas();
        void heartbeat();
        void send_to_replicas(std::string data);
        void always_listening();
};

void send_ack(int sock, sockaddr_in target);

#endif