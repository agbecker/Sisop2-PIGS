#include "Discovery.h"
using namespace std;

void Discovery::set_str(string* str) {
    str_pointer = str;
}

void Discovery::run() {
    cout << *(str_pointer) << endl;
    this_thread::sleep_for(chrono::seconds(1));
    cout << *(str_pointer) << endl;
}

void Discovery::treat_request (string message, struct sockaddr_in cli_addr) {

    string client_ip = inet_ntoa(cli_addr.sin_addr); // Salva IP recebido como string

    if (message != DISCOVERY_ASK)
        return;

    // Envia ACK
    char ack[BUFFER_SIZE] = DISCOVERY_REPLY;
    int n = sendto(sockfd, ack, strlen(ack), 0, (const struct sockaddr *) &cli_addr, sizeof(struct sockaddr_in));

    // Adiciona cliente à fila de novos clientes, a ser tratado na classe Server
    mutex_add_client->lock();
    clients_to_add->push(client_ip);
    mutex_add_client->unlock();
}

void Discovery::awaitRequest() {
    socklen_t clilen = sizeof(struct sockaddr_in); // Tamanho do IP do cliente
    struct sockaddr_in serv_addr, cli_addr; // Endereços IP do servidor e do cliente
    

    // Cria socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("ERROR opening socket");
        return;
    }

    // Inicializa socket (IPv4, porta BROADCAST_PORT, aceita conexão de qualquer lugar)
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(BROADCAST_PORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(serv_addr.sin_zero), 8);    

    // Configura socket para receber mensagens em broadcast
    int bc_enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &bc_enable, sizeof(bc_enable)) < 0) {
        perror("ERROR on broadcast enabling");
        close(sockfd);
        return;
    }

    // Faz bind no socket à porta
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) {
		perror("ERROR on binding");
        close(sockfd);
        return;
    }

    while(true) {
        bzero(buf, BUFFER_SIZE); // Limpa buffer

        // Recebe mensagem do socket (bloqueante, espera até receber)
        int n = recvfrom(sockfd, buf, BUFFER_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen);

        if (n<0) {
            perror("ERROR on recvfrom");
            continue;
        }

        string message = buf;
        thread ack_thread(&Discovery::treat_request, this, message, cli_addr);
        ack_thread.detach();
    }

    close(sockfd);
    return;

}

// Informa a todos os clientes sobre o novo servidor principal
void Discovery::update_clients_about_main(const std::vector<std::string> ips) {
    for (const auto& ip : ips) {
        std::thread(notify_client_new_server, ip).detach();
    }
}

static void notify_client_new_server(const std::string& ip) {
    int sock;
    char buf[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return;
    }

    // Timeout de 1 segundo
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in cli_addr{};
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(BROADCAST_PORT);
    inet_aton(ip.c_str(), &cli_addr.sin_addr);

    while (true) {
        // Envia mensagem de novo servidor
        sendto(
            sock,
            NEW_SERVER_MESSAGE,
            strlen(NEW_SERVER_MESSAGE),
            0,
            (struct sockaddr*)&cli_addr,
            sizeof(cli_addr)
        );

        // Aguarda ACK
        struct sockaddr_in from_addr{};
        socklen_t from_len = sizeof(from_addr);
        int n = recvfrom(
            sock,
            buf,
            BUFFER_SIZE - 1,
            0,
            (struct sockaddr*)&from_addr,
            &from_len
        );

        if (n < 0) {
            // Timeout → retry
            continue;
        }

        buf[n] = '\0';

        // Confere IP de origem
        if (from_addr.sin_addr.s_addr != cli_addr.sin_addr.s_addr)
            continue;

        // Confere ACK correto
        if (strcmp(buf, ACK_NEW_SERVER) == 0) {
            break;
        }
        // Qualquer outra coisa → ignora e continua
    }

    close(sock);
}
