#include "Process.h"

using namespace std;

void Process::run() {
    socklen_t clilen = sizeof(struct sockaddr_in); // Tamanho do IP do cliente
	struct sockaddr_in serv_addr, cli_addr; // Endereços IP do servidor e do cliente
	char buf[256];
		
    // Cria socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("ERROR opening socket");
        return;
    }

    // Debug
	printf("passou pela criacao do socket\n");

    // Inicializa socket (IPv4, porta PORT, recebe de qualquer endereço)
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(serv_addr.sin_zero), 8);    
	
	// Faz bind no socket à porta
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) {
		perror("ERROR on binding");
        close(sockfd);
        return;
    }

    // Debug
	printf("passou pelo bind\n");


	while(true) {
		bzero(buf, BUFFER_SIZE); // Limpa o buffer
		
		// Recebe mensagem do socket (bloqueante, espera até receber)
        int n = recvfrom(sockfd, buf, BUFFER_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen);
		
		if (n<0) {
            perror("ERROR on recvfrom");
            continue;
        }

        // Debug
		printf("Recebi o seguinte datagrama: %s\n", buf);
		
		// Processa a requisição
        string message = buf;
        thread t_process (&Process::processTransaction, this, message, cli_addr);
        t_process.detach();
	}
	
	close(sockfd);
}


void Process::processTransaction(string message, struct sockaddr_in cli_addr) {
    return;
}