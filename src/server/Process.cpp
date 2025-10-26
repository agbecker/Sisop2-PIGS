#include "Process.h"

using json = nlohmann::json;
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

	while(true) {
		bzero(buf, BUFFER_SIZE); // Limpa o buffer
		
		// Recebe mensagem do socket (bloqueante, espera até receber)
        int n = recvfrom(sockfd, buf, BUFFER_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen);
		
		if (n<0) {
            perror("ERROR on recvfrom");
            continue;
        }
		
		// Processa a requisição
        string message = buf;
        thread t_process (&Process::processTransaction, this, message, cli_addr);
        t_process.detach();
	}
	
	close(sockfd);
}


void Process::processTransaction(string message, struct sockaddr_in cli_addr) {
    // Obtem os dados
    json request = json::parse(message);

    string ip_sender = inet_ntoa(cli_addr.sin_addr);
    string ip_receiver = request["receiver"];
    int amount = request["amount"];
    int seq_sender = request["sequence"];

    // Acessa a lista
    mtx_clients->lock();

    // Verifica numero de sequencia
    int seq_server = (*clients)[ip_sender].seq_num;
    if(seq_server != seq_sender-1) {

        // Registra duplicata na fila de eventos
        if(seq_server <= seq_sender) {
            ServerStats cur_stats = *stats;
            Event event(amount, seq_sender, ip_sender, ip_receiver, true, cur_stats);
            mtx_events->lock();
            events->push(event);
            mtx_events->unlock();
        }

        sendReply(cli_addr, RR_NUMBER, (*clients)[ip_sender].balance, (*clients)[ip_sender].seq_num);
        mtx_clients->unlock();
        return;
    }

    // Verifica se o valor transferido é zero (usuário só quer ver o saldo)
    // nesse caso não analisa se o IP destino existe
    if(amount == 0) {
        sendReply(cli_addr, RR_OK, (*clients)[ip_sender].balance, (*clients)[ip_sender].seq_num);
        mtx_clients->unlock();
        return;
    }

    // Verifica se o destinatário consta na lista
    if(clients->count(ip_receiver) == 0) {
        sendReply(cli_addr, RR_NOTONLIST, (*clients)[ip_sender].balance, (*clients)[ip_sender].seq_num);
        mtx_clients->unlock();
        return;
    }

    // Verifica se saldo é suficiente
    if((*clients)[ip_sender].balance < amount) {
        sendReply(cli_addr, RR_BALANCE, (*clients)[ip_sender].balance, (*clients)[ip_sender].seq_num);
        mtx_clients->unlock();
        return;
    }

    // Transfere o dinheiro
    (*clients)[ip_sender].balance -= amount;
    (*clients)[ip_receiver].balance += amount;
    (*clients)[ip_sender].seq_num++;

    // Atualiza dados do servidor
    stats->num_transactions++;
    stats->total_transferred += amount;

    int new_balance = (*clients)[ip_sender].balance; // Salva novo saldo do cliente para retornar

    // Registra na fila de eventos
    ServerStats cur_stats = *stats;
    Event event(amount, seq_sender, ip_sender, ip_receiver, false, cur_stats);
    mtx_events->lock();
    events->push(event);
    mtx_events->unlock();

    // Responde com ok
    sendReply(cli_addr, RR_OK, (*clients)[ip_sender].balance, (*clients)[ip_sender].seq_num);
    
    // Libera acesso à lista
    mtx_clients->unlock();
}

void Process::sendReply(struct sockaddr_in cli_addr, int status, int new_balance, int seq_num) {
    char buf[BUFFER_SIZE];
    int sockfd, n;

    // Cria o socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        return;
    }

    // Cria a mensagem com resposta
    json ack = {
        {"status", status},
        {"balance", new_balance},
        {"sequence", seq_num}
    };

    // Converte JSON para string
    string message = ack.dump(4);
    bzero(buf, BUFFER_SIZE);
    strcpy(buf, message.c_str());

    // Envia a resposta
    n = sendto(sockfd, buf, BUFFER_SIZE, 0, (const struct sockaddr *) &cli_addr, sizeof(struct sockaddr_in));
    if (n < 0) {
		perror("ERROR sendto");
        return;
    }

    close(sockfd);
}