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
    // Debug
    string client_ip = inet_ntoa(cli_addr.sin_addr); // Salva IP recebido como string
    cout << "Recebi mensagem do " << client_ip << endl;
    cout << "A mensagem diz: " << message << endl;

    if (message != "WHERE IS SERVER OINK")
        return;

    // Envia ACK
    char ack[BUFFER_SIZE] = "SERVER HERE OINK";
    int n = sendto(sockfd, ack, strlen(ack), 0, (const struct sockaddr *) &cli_addr, sizeof(struct sockaddr_in));
}

void Discovery::awaitRequest() {
    socklen_t clilen = sizeof(struct sockaddr_in); // Tamanho do IP do cliente
    struct sockaddr_in serv_addr, cli_addr; // Endereços IP do servidor e do cliente
    

    // Cria socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("ERROR opening socket");
        return;
    }
    printf("Passou pela criacao do socket\n");

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

