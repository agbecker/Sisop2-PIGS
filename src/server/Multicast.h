#ifndef MULTICAST_H
#define MULTICAST_H

#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <queue>
#include <mutex>

#define MCAST_IP "239.0.0.1"

#define HEARTBEAT "OINK OINK"
#define HEARTBEAT_PERIOD 100

class Multicast {
    private:
        int sock;
        sockaddr_in group;
        int heartbeat_counter;
        std::mutex mtx_heartbeat_counter;
        std::string newest_update;
        bool election_in_progress;
        bool higher_response_received;

    public:
        Multicast(): heartbeat_counter(0), election_in_progress(false) {};
        void init();
        void find_others(bool* is_only_server);
        void heartbeat();
        void send_to_replicas(std::string data);
        void always_listening();
        void monitor_rm_heartbeat();
        void start_election() 
};

#endif