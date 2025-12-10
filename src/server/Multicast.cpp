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

void Multicast::find_others(bool* is_only_server) {
    const char* msg = MC_DISCOVERY_ASK;

    // Envia a mensagem multicast
    sendto(sock, msg, strlen(msg), 0, (sockaddr*)&group, sizeof(group));

    // Configura timeout de 10ms
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);

    timeval tv{};
    tv.tv_sec = 0;
    tv.tv_usec = 10000; // 10 ms

    // Espera por qualquer resposta
    int ret = select(sock + 1, &readfds, nullptr, nullptr, &tv);

    if (ret > 0 && FD_ISSET(sock, &readfds)) {
        // Alguma resposta chegou
        char buffer[256];
        sockaddr_in sender{};
        socklen_t sender_len = sizeof(sender);
        ssize_t n = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                             (sockaddr*)&sender, &sender_len);
        if (n > 0) {
            buffer[n] = '\0';
            *is_only_server = false; // alguém respondeu
            return;
        }
    }

    // Nenhuma resposta
    *is_only_server = true;
}

void send_ack(int sock, sockaddr_in target) {
    const char* msg = MC_DISCOVERY_ACK;
    sendto(sock, msg, strlen(msg), 0,
           (sockaddr*)&target, sizeof(target));
}

// Aguarda novas réplicas mandarem mensagem e responde
void Multicast::welcome_new_replicas() {
    char buffer[256];
    while (true) {
        sockaddr_in sender{};
        socklen_t sender_len = sizeof(sender);

        ssize_t n = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                             (sockaddr*)&sender, &sender_len);

        if (n <= 0) continue;

        buffer[n] = '\0';

        if (strcmp(buffer, MC_DISCOVERY_ASK) == 0) {
            // Cria uma thread para enviar a resposta
            thread t(send_ack, sock, sender);
            t.detach(); // thread independente
        }
    }
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

void Multicast::await_backup_data() {
    char buffer[8192]; // tamanho razoável para JSON

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
        std::string json_data(buffer);

        // Encaminha para atualização dos dados no backup
        // update_client_data(json_data);
        cout << buffer << endl;
    }
}
