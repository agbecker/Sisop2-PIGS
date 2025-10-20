#include "Server.h"
using namespace std;

int main() {
    total_balance = 0;

    thread t_add_clients(add_clients);

    Discovery discovery;
    discovery.clients_to_add = &clients_to_add;
    discovery.mutex_add_client = &mutex_new_clients;
    thread t_discovery(&Discovery::awaitRequest, &discovery);

    Process process(&clients);
    thread t_process(&Process::run, &process);

    // Debug
    clients_to_add.push("1.2.3.4");

    while(!t_discovery.joinable() && !t_process.joinable());
    t_discovery.join();
    t_process.join();

    return 0;
}


// Monitora a fila de novos clientes adicionados por Discovery
// e adiciona-os à lista de clientes
void add_clients() {
    while (true) {
        while (clients_to_add.empty()); // Aguarda algo ser adicionado

        // Pega o IP do cliente na cabeça da fila
        mutex_new_clients.lock();
        string ip = clients_to_add.front();
        clients_to_add.pop();
        mutex_new_clients.unlock();

        // Verifica se já consta na lista
        if (clients.find(ip) != clients.end()) {
            

            // Debug
            cout << "O cliente " << ip << " já consta na lista" << endl;

            continue; // Se já consta, não faz nada
        }

        // Adiciona à lista
        ClientData new_client(ip,STARTING_BALANCE,0); // Cria novo cliente
        mutex_client_list.lock();
        clients.insert({ip, new_client});
        mutex_client_list.unlock();

        // Atualiza o saldo
        total_balance += STARTING_BALANCE;

        // Debug
        cout << "Adicionei o cliente " << ip << " à lista" << endl;

    }
}