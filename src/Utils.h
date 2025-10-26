#ifndef UTILS_H
#define UTILS_H
#include <string>

// Struct usada para comunicação entre threads do cliente
typedef struct RequestReply
{   int status;
    int value;
    int seq_num;
    std::string destination;
} RequestReply;

// Status possíveis:
#define RR_INVALID 1    // O dado não tem significado, não deve ser processado
#define RR_SEND 2       // O dado é válido e deve ser enviado ao servidor
#define RR_WAITING 3    // Aguardando resposta do servidor
#define RR_OK 4         // O dado foi processado corretamente
#define RR_BALANCE 5    // Erro no processamento por saldo insuficiente
#define RR_NUMBER 6     // Erro no processamento por número de sequência errado
#define RR_IGNORED 7    // Sem resposta do servidor
#define RR_CONNECT 8    // Erro de conexão
#define RR_NOTONLIST 9  // IP destinatário não consta na lista de clientes

#define STARTING_BALANCE 1000

#define TIMEOUT 10      // Tempo limite de time-out (em ms)

#define DISCOVERY_ASK "WHERE IS SERVER OINK"
#define DISCOVERY_REPLY "SERVER HERE OINK"

// Armazena os dados numéricos do servidor em um dado momento
typedef struct ServerStats
{
    unsigned int num_clients;
    long unsigned int total_transferred;
    unsigned int num_transactions;
    ServerStats(): num_clients(0), total_transferred(0), num_transactions(0) {};
} ServerStats;

// Struct usada para comunicação entre threads do servidor
typedef struct Event
{   int value;
    int seq_num;
    std::string origin;
    std::string destination;
    bool duplicate;
    ServerStats stats;
    Event(int v, int n, std::string o, std::string d, bool dp, ServerStats st): value(v), seq_num(n), origin(o), destination(d), duplicate(dp), stats(st) {};
} Event;

#endif