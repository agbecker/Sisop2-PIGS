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
    rr->status = RR_OK;
}