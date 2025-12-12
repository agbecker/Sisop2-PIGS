#include "Process.h"

using json = nlohmann::json;
using namespace std;

void Process::run() {
    while(true) {
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
        string ack = sendToServer(message);

        if (ack.empty()) {
            // falha de conexão ou timeout
            // evita crash ao ler o json (vazio)
            continue;
        }

        // Faz parsing da resposta
        json reply = json::parse(ack);

        int status = reply["status"];

        // Processamento ok
        if(status == RR_OK) {
            this->num_seq = reply["sequence"];
            this->num_seq++;
            rr->value = reply["balance"];
            rr->seq_num = reply["sequence"];
            rr->status = status;
            continue;
        }

        // Erro de destinatário inválido
        if(status == RR_NOTONLIST) {
            rr->value = reply["balance"];
            rr->seq_num = reply["sequence"];
            rr->status = status;
            continue;
        }

        // Erro de saldo insuficiente
        if(status == RR_BALANCE) {
            rr->value = reply["balance"];
            rr->seq_num = reply["sequence"];
            rr->status = status;
            continue;
        }
        
        // Erro de número de sequência
        // Na prática não deveria acontecer, 
        // mas presume que não imprimiu a última requisição
        // e imprime ela. Senão, descarta a última e ajusta o número
        if(status == RR_NUMBER) {
            // Última não processada
            if(rr->seq_num == reply["sequence"]) {
                this->num_seq++;
                rr->value = reply["balance"];
                rr->seq_num = reply["sequence"];
                rr->status = RR_OK;
            }

            // Caso contrário, descarta
            else {
                rr->seq_num = reply["sequence"];
                this->num_seq = rr->seq_num+1;
                rr->value = reply["balance"];
                rr->status = RR_NUMBER;
            }
        }

    }
}

string Process::sendToServer(string request) {
    int sockfd, n;
    struct sockaddr_in server;
    char buf[BUFFER_SIZE];

    // Inicializa socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("ERROR opening socket");
        rr->status = RR_CONNECT;
        return "";
    }

    // Configura timeout para recvfrom
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = TIMEOUT;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt timeout");
        close(sockfd);
        return "";
    }

    // Prepara conexão
    server.sin_family = AF_INET;     
	server.sin_port = htons(port);
    server.sin_addr = this->serv_addr;

    // Envia
    n = -1;
    while(n < 0){
        // Prepara mensagem
        bzero(buf, BUFFER_SIZE);
        strcpy(buf,request.c_str());

        n = sendto(sockfd, buf, BUFFER_SIZE, 0, (const struct sockaddr *) &server, sizeof(struct sockaddr_in));
        if (n < 0) {
            perror("ERROR sendto");
            rr->status = RR_CONNECT;
            return "";
        }

        n = recvfrom(sockfd, buf, BUFFER_SIZE, 0, nullptr, nullptr);
        if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("ERROR recvfrom");
            rr->status = RR_CONNECT;
            return "";
        }
    }  

    close(sockfd);

    return buf;
}