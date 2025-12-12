#ifndef MULTICAST_H
#define MULTICAST_H

#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <queue>
#include <mutex>
#include <sstream>
#include <atomic>
#include <functional>

#define MCAST_IP "239.0.0.1"

#define HEARTBEAT "OINK OINK"
#define HEARTBEAT_PERIOD 100
#define ELECTION_TIMEOUT HEARTBEAT_PERIOD * 3.0
#define MSG_ELECTION "ELECTION "                // inicia eleição, seguido do ID do processo
#define MSG_ELECTION_OK "OK "                   // resposta a eleição, seguido do ID do processo
#define MSG_ELECTION_COORDINATOR "COORDINATOR " // Avisa novo coordenador, seguido do ID do processo
#define MSG_STATE_REQUEST "GIMME STATE"         // pedido de estado atual

class Multicast {
    private:
        int sock;
        sockaddr_in group;
        int heartbeat_counter;
        std::mutex mtx_heartbeat_counter;
        std::string newest_update;

        // variables for election
        int my_id;
        std::atomic<bool> election_running;
        std::atomic<bool> backup_running;
        std::atomic<bool> manager_running;
        std::function<void(std::string)> on_update_received;
        std::function<std::string()> on_state_request;

    public:
        Multicast(int id): heartbeat_counter(0), my_id(id), election_running(false), backup_running(false), manager_running(false) {};
        void init();
        void set_on_update_received(std::function<void(std::string)> callback) { on_update_received = callback; }
        void set_on_state_request(std::function<std::string()> callback) { on_state_request = callback; }
        void request_state();
        void find_others(bool* is_only_server);
        void heartbeat();
        void send_to_replicas(std::string data);
        void always_listening();
        void monitor_rm_heartbeat();
        
        void start_backup(){ backup_running = true; manager_running = false; }
        void stop_backup(){ backup_running = false; }

        void start_manager(){ manager_running = true; backup_running = false; }
        void stop_manager(){ manager_running = false; }
        bool is_manager(){ return manager_running; }
        
        void partake_in_election(int sender_id);
        void start_election();

    };

#endif