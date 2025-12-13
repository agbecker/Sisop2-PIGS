#include "Discovery.h"
using namespace std;


bool discoverServer(struct sockaddr_in &serv_addr) {
    int sockfd; // ID do socket
    socklen_t servlen = sizeof(struct sockaddr_in); // Tamanho do IP do servidor
    struct hostent *server; // Servidor
    char buf[BUFFER_SIZE]; // Mensagem

    // Cria socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("ERROR opening socket");
        return false;
    }

    // Configura socket para enviar mensagens em broadcast
    int bc_enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &bc_enable, sizeof(bc_enable)) < 0) {
        perror("ERROR on broadcast enabling");
        close(sockfd);
        return false;
    }

    // Configura timeout para recvfrom
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = TIMEOUT;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt timeout");
        close(sockfd);
        return false;
    }

    // Inicializa socket (IPv4, porta BROADCAST_PORT, envia para o IP de broadcast)
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(BROADCAST_PORT);
	serv_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    bzero(&(serv_addr.sin_zero), 8);

    int n = -1;
    while(n < 0) {
        // Envia a mensagem
        strcpy(buf, DISCOVERY_ASK);
        n = sendto(sockfd, buf, strlen(buf), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));

        // Aguarda o ACK
        bzero(buf, BUFFER_SIZE);
        n = recvfrom(sockfd, buf, BUFFER_SIZE, 0, (struct sockaddr *) &serv_addr, &servlen);
        
        // Verifica que a resposta foi realmente do servidor
        string reply = buf;
        if(reply != DISCOVERY_REPLY) {
            n = -1;
        }
    }

    close(sockfd);
    return true;

}

void awaitNewServer(in_addr* serv_addr) {
    int sockfd;
    char buf[BUFFER_SIZE];
    struct sockaddr_in local_addr{}, from_addr{};
    socklen_t from_len = sizeof(from_addr);

    // Cria socket UDP
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening discovery socket");
        return;
    }

    // Permite broadcast
    int bc_enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &bc_enable, sizeof(bc_enable)) < 0) {
        perror("ERROR enabling broadcast");
        close(sockfd);
        return;
    }

    // Configura socket para permitir reuso de endereço e porta
    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("ERROR enabling SO_REUSEADDR");
        close(sockfd);
        return;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
        perror("ERROR enabling SO_REUSEPORT");
        close(sockfd);
        return;
    }

    // Bind na porta de discovery
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(NOTIFICATION_PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(local_addr.sin_zero), 8);

    if (bind(sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        perror("ERROR binding discovery socket");
        close(sockfd);
        return;
    }

    while (true) {
        bzero(buf, BUFFER_SIZE);

        int n = recvfrom(
            sockfd,
            buf,
            BUFFER_SIZE,
            0,
            (struct sockaddr*)&from_addr,
            &from_len
        );

        if (n < 0) {
            perror("ERROR recvfrom (discovery)");
            continue;
        }

        string msg(buf);

        // Ignora mensagens irrelevantes
        if (msg != NEW_SERVER_MESSAGE)
            continue;

        // Atualiza servidor atual
        *serv_addr = from_addr.sin_addr;

        // Envia ACK
        const char ack[] = ACK_NEW_SERVER;
        sendto(
            sockfd,
            ack,
            strlen(ack),
            0,
            (struct sockaddr*)&from_addr,
            from_len
        );
    }

    close(sockfd);
}