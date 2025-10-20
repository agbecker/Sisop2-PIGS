#include "Process.h"

using json = nlohmann::json;
using namespace std;

void Process::run() {
    while(rr->status!=RR_SEND);
    string receiver_ip = rr->destination;
    int amount = rr->value;
    json request = {
        {"receiver", receiver_ip},
        {"amount", amount},
        {"sequence", num_seq}
    };

    // Converte JSON para string
    string message = request.dump(4);

    // Envia para o servidor
    rr->status = RR_WAITING;
    sendToServer(message);
}

void Process::sendToServer(string request) {
    int sockfd, n;
    struct sockaddr_in server;
    char buf[BUFFER_SIZE];

    // Inicializa socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("ERROR opening socket");
        rr->status = RR_CONNECT;
        return;
    }

    // Prepara conexão
    server.sin_family = AF_INET;     
	server.sin_port = htons(PORT);
    server.sin_addr = this->serv_addr;

    // Prepara mensagem
    bzero(buf, BUFFER_SIZE);
    strcpy(buf,request.c_str());

    // Envia
    n = sendto(sockfd, buf, BUFFER_SIZE, 0, (const struct sockaddr *) &server, sizeof(struct sockaddr_in));
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("ERROR sendto");
        rr->status = RR_CONNECT;
        return;
    }

    n = recvfrom(sockfd, buf, BUFFER_SIZE, 0, nullptr, nullptr);
    if (n < 0) {
		perror("ERROR recvfrom");
        rr->status = RR_CONNECT;
        return;
    }

    // Debug
    rr->status = RR_OK;
}