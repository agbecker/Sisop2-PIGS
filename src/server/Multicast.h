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
        int id;

    public:
        Multicast(int i):id(i), heartbeat_counter(0), election_in_progress(false), higher_response_received(false) {};
        void init();
        void find_others(bool* is_only_server);
        void heartbeat();
        void send_to_replicas(std::string data);
        void always_listening();
        void monitor_rm_heartbeat();
        void start_election();
        void on_receive_election(int sender_id);
        void on_receive_ok(int sender_id);
        void on_receive_coordinator(int leader_id);
};

#endif