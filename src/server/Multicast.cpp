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
    int loop = 0; // 0 desativa, 1 ativa
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

    while (true) {
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
    char buffer[8192];

    while (true) {
        sockaddr_in sender{};
        socklen_t sender_len = sizeof(sender);

        ssize_t n = recvfrom(
            sock,
            buffer,
            sizeof(buffer) - 1,
            0,
            (sockaddr*)&sender,
            &sender_len
        );

        if (n <= 0)
            continue;

        buffer[n] = '\0';
        std::string received_msg(buffer);

        // HEARTBEAT
        if(received_msg == HEARTBEAT) {
            mtx_heartbeat_counter.lock();
            heartbeat_counter = 0;
            mtx_heartbeat_counter.unlock();
            continue;
        }

        // ELECTION <id>
        if (received_msg.rfind("ELECTION ", 0) == 0) {
            int sender_id = std::stoi(received_msg.substr(9));
            on_receive_election(sender_id);
            continue;
        }

        // OK <id>
        if (received_msg.rfind("OK ", 0) == 0) {
            int sender_id = std::stoi(received_msg.substr(3));
            on_receive_ok(sender_id);
            continue;
        }

        // COORDINATOR <id>
        if (received_msg.rfind("COORDINATOR ", 0) == 0) {
            int leader_id = std::stoi(received_msg.substr(12));
            on_receive_coordinator(leader_id);
            continue;
        }

        // Atualização normal
        newest_update = received_msg;
    }
}


// Conta o número de períodos sem heartbeat, para detectar falha do RM
void Multicast::monitor_rm_heartbeat() {
    while (true) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(HEARTBEAT_PERIOD)
        );

        mtx_heartbeat_counter.lock();
        heartbeat_counter++;

        if (heartbeat_counter >= 3) {
            heartbeat_counter = 0;
            mtx_heartbeat_counter.unlock();

            // Agora dispara o ciclo de eleição
            start_election();
            continue;
        }

        mtx_heartbeat_counter.unlock();
    }
}

// Conta o número de períodos sem heartbeat, para detectar falha do RM
void Multicast::monitor_rm_heartbeat() {
    while (true) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(HEARTBEAT_PERIOD)
        );

        mtx_heartbeat_counter.lock();
        heartbeat_counter++;

        if (!election_in_progress && heartbeat_counter >= 3) {
            heartbeat_counter = 0; // evita múltiplos disparos
            mtx_heartbeat_counter.unlock();

            this->start_election();

            // Debug
            // cout << "MORREU" << endl;
            continue;
        }

        mtx_heartbeat_counter.unlock();
    }
}

// Inicia uma eleição
void Multicast::start_election() {
    if (sock < 0) return;
    this->election_in_progress = true;
    this->higher_response_received = false;

    std::string msg = "ELECTION " + std::to_string(id);

    ssize_t sent = sendto(
        sock,
        msg.c_str(),
        msg.size(),
        0,
        (sockaddr*)&group,
        sizeof(group)
    );

    if (sent < 0) {
        perror("sendto (start_election)");
    }
}

// Participa da eleição
void Multicast::on_receive_election(int sender_id) {
    // Ignorar mensagens minhas
    if (sender_id == this->id)
        return;

    // Caso o remetente tenha ID menor que o meu:
    // Sou mais forte → devo responder OK
    if (sender_id < this->id) {

        // Envia "OK <id>"
        std::string ok_msg = "OK " + std::to_string(this->id);
        ssize_t sent = sendto(
            sock,
            ok_msg.c_str(),
            ok_msg.size(),
            0,
            (sockaddr*)&group,
            sizeof(group)
        );

        if (sent < 0) {
            perror("sendto (on_receive_election OK)");
        }

        // Se eu ainda não estou em eleição, começo a minha
        if (!this->election_in_progress) {
            this->start_election();
        }

        return;
    }

    // Caso o remetente tenha ID maior que o meu:
    // Ele é mais forte → deixo que ele continue a eleição
    if (sender_id > this->id) {
        // Não faço nada: processo mais forte segue com a eleição
        return;
    }
}


void Multicast::on_receive_ok(int sender_id) {
    // Ignora OKs enviados por mim mesmo se vazarem no multicast
    if (sender_id == this->id)
        return;

    // Registra que recebeu OK de um processo mais forte
    this->higher_response_received = true;

    // Se eu estava esperando respostas da eleição, agora sei que não sou o mais forte
    // Apenas aguardo COORDINATOR; não inicio nada e não me declaro líder
}


void Multicast::on_receive_coordinator(int new_leader_id) {
    // Atualiza o novo coordenador conhecido
    this->current_leader_id = new_leader_id;

    // A eleição terminou
    this->election_in_progress = false;
    this->higher_response_received = false;

    // Debug opcional
    std::cout << "Novo coordenador é " << new_leader_id << std::endl;

    // Se eu era o líder antigo ou estava me candidatando,
    // agora eu apenas aceito o novo líder e continuo normal.
}