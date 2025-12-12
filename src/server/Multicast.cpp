#include "Multicast.h"

using namespace std;

// Ingressa no grupo multicast de comunicação entre as réplicas
void Multicast::init() {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return;
    }

    // Permite múltiplos binds na mesma porta
    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
               &reuse, sizeof(reuse));

    // Configura que não recebe as próprias mensagens
    // não faz muita diferença na implementação atual, mas ajuda a debugar
    int loop = 1; // 0 desativa, 1 ativa
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = htons(12345);
    local.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock,
             (sockaddr*)&local, sizeof(local)) < 0) {
        perror("bind");
        return;
    }

    // Define o grupo multicast uma vez
    memset(&group, 0, sizeof(group));
    group.sin_family = AF_INET;
    group.sin_port = htons(12345);
    inet_pton(AF_INET, MCAST_IP, &group.sin_addr);

    // Adiciona ao grupo multicast
    ip_mreq mreq{};
    inet_pton(AF_INET, MCAST_IP, &mreq.imr_multiaddr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
}

// Aguarda ver se já tem um servidor principal mandando heartbeats
// Senão, assume o papel de principal
void Multicast::find_others(bool* is_only_server) {
    // Configura timeout total
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);

    timeval tv{};
    tv.tv_sec = 0;
    tv.tv_usec = HEARTBEAT_PERIOD * 1000 * 4;

    int ret = select(sock + 1, &readfds, nullptr, nullptr, &tv);

    if (ret > 0 && FD_ISSET(sock, &readfds)) {
        char buffer[256];
        sockaddr_in sender{};
        socklen_t sender_len = sizeof(sender);

        ssize_t n = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                             (sockaddr*)&sender, &sender_len);

        if (n > 0) {
            buffer[n] = '\0';

            // Agora só aceita se for HEARTBEAT
            if (strcmp(buffer, HEARTBEAT) == 0) {
                *is_only_server = false;
                return;
            }
        }
    }

    // Timeout ou mensagem inválida
    *is_only_server = true;
}

// Dá um sinal periódico às réplicas para avisar que o servidor ainda está ativo
void Multicast::heartbeat() {
    const char* msg = HEARTBEAT;

    // só envia enquanto for o manager
    while (manager_running) {
        sendto(sock, msg, strlen(msg), 0,
               (sockaddr*)&group, sizeof(group));

        std::this_thread::sleep_for(std::chrono::milliseconds(HEARTBEAT_PERIOD));
    }
}

// Envia string JSON com os dados dos clientes para o grupo multicast
void Multicast::send_to_replicas(std::string data) {
    if (sock < 0) return;

    ssize_t sent = sendto(
        sock,
        data.c_str(),
        data.size(),
        0,
        (sockaddr*)&group,
        sizeof(group)
    );

    if (sent < 0) {
        perror("sendto (send_to_replicas)");
    }

}

// Escuta mensagens no multicast e encaminha pro tratamento correto
void Multicast::always_listening() {
    char buffer[8192]; // tamanho razoável para JSON

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    while (backup_running || manager_running) {
        sockaddr_in sender{};
        socklen_t sender_len = sizeof(sender);

        ssize_t n = recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&sender, &sender_len);

        if (n <= 0)
            continue;

        buffer[n] = '\0';
        std::string received_msg(buffer);

        if(received_msg == HEARTBEAT){
            if (backup_running) {
                mtx_heartbeat_counter.lock();
                heartbeat_counter = 0;
                mtx_heartbeat_counter.unlock();
            }
        }
        else if(received_msg.rfind(MSG_ELECTION, 0) == 0){
            int sender_id = stoi(received_msg.substr(string(MSG_ELECTION).length()));
            partake_in_election(sender_id);
        }
        else if(received_msg.rfind(MSG_ELECTION_OK, 0) == 0){
            election_running = false;
        }
        else if(received_msg.rfind(MSG_ELECTION_COORDINATOR, 0) == 0){
            int new_leader_id = stoi(received_msg.substr(string(MSG_ELECTION_COORDINATOR).length()));
            
            // Se alguém virou coordenador, a eleição acabou
            election_running = false;

            if(new_leader_id == my_id){
                if (backup_running) stop_backup();
            }
            else{
                if (manager_running) stop_manager();

                if (backup_running) {
                    mtx_heartbeat_counter.lock();
                    heartbeat_counter = 0;
                    mtx_heartbeat_counter.unlock();
                }
            }
        }
        else if(received_msg.rfind(MSG_STATE_REQUEST, 0) == 0){
            if(manager_running && on_state_request) {
                string state_json = on_state_request();
                send_to_replicas(state_json);
            }
        }
        else{
            newest_update = received_msg;
            if(on_update_received) on_update_received(received_msg);
        }
    }
}

// Conta o número de períodos sem heartbeat, para detectar falha do RM
void Multicast::monitor_rm_heartbeat() {
    while (backup_running) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(HEARTBEAT_PERIOD)
        );

        mtx_heartbeat_counter.lock();
        heartbeat_counter++;

        if (heartbeat_counter >= 3) {
            heartbeat_counter = 0; // evita múltiplos disparos
            mtx_heartbeat_counter.unlock();

            start_election();

            // Debug
            // cout << "MORREU" << endl;
            continue;
        }

        mtx_heartbeat_counter.unlock();
    }
}

// Inicia uma eleição entre as réplicas
// e cria uma thread para aguardar respostas
void Multicast::start_election(){
    if(election_running) return;
    election_running = true;

    string election_msg = MSG_ELECTION + to_string(my_id);    
    send_to_replicas(election_msg);

    // Thread para esperar respostas (Timeout)
    thread t_timeout([this]() {
        this_thread::sleep_for(chrono::seconds(2));

        if (this->election_running) {
            
            // Ninguém maior respondeu (não recebi OK). fiz bullying com todos!
            string coord_msg = MSG_ELECTION_COORDINATOR + to_string(this->my_id);
            this->send_to_replicas(coord_msg);
            
            this->election_running = false;
            this->stop_backup();
        } else {
            // fizram bullying comigo :(
            cout << "I lost the election." << endl;
        }
    });
    t_timeout.detach();
}

void Multicast::partake_in_election(int sender_id){
    if(sender_id < my_id){
        string ok_msg = MSG_ELECTION_OK + to_string(my_id);
        send_to_replicas(ok_msg);
        
        if (manager_running) {
             string coord_msg = MSG_ELECTION_COORDINATOR + to_string(my_id);
             send_to_replicas(coord_msg);
        } else {
             start_election();
        }
    }
}

void Multicast::request_state() {
    string msg = MSG_STATE_REQUEST;
    send_to_replicas(msg);
}
