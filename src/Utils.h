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
#define RR_SEQ 6        // Erro no processamento por número de sequência errado











#endif