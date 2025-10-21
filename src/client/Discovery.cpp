#include "Discovery.h"
using namespace std;


bool discoverServer(struct sockaddr_in &serv_addr) {
    int sockfd; // ID do socket
    socklen_t servlen = sizeof(struct sockaddr_in); // Tamanho do IP do servidor
    struct hostent *server; // Servidor
    char buf[BUFFER_SIZE] = "WHERE IS SERVER OINK"; // Mensagem

    // Cria socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("ERROR opening socket");
        return false;
    }
    printf("Passou pela criacao do socket\n");

    // Configura socket para enviar mensagens em broadcast
    int bc_enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &bc_enable, sizeof(bc_enable)) < 0) {
        perror("ERROR on broadcast enabling");
        close(sockfd);
        return false;
    }

    // Inicializa socket (IPv4, porta BROADCAST_PORT, envia para o IP de broadcast)
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(BROADCAST_PORT);
	serv_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    bzero(&(serv_addr.sin_zero), 8);

    // Envia a mensagem
    int n = sendto(sockfd, buf, strlen(buf), 0, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));

    // Aguarda o ACK
    bzero(buf, BUFFER_SIZE);
    n = recvfrom(sockfd, buf, BUFFER_SIZE, 0, (struct sockaddr *) &serv_addr, &servlen);

    // Debug
    string server_ip = inet_ntoa(serv_addr.sin_addr);
    cout << "O servidor tem IP " << server_ip << endl;
    cout << "Ele respondeu: " << buf << endl;

    close(sockfd);
    return true;

}