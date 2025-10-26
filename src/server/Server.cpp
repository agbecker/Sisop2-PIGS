#include "Server.h"
using namespace std;

int main() {
    // Thread para a interface do servidor
    Interface interface(&events, &mtx_events);
    thread t_interface(&Interface::run, &interface);

    // Thread para monitorar a adição de clientes à lista
    thread t_add_clients(add_clients);

    // Thread para descoberta de novos clientes
    Discovery discovery(clients_to_add, mutex_new_clients);
    thread t_discovery(&Discovery::awaitRequest, &discovery);

    // Thread para processamento de requisições
    Process process(&clients, &mutex_client_list, &events, &mtx_events, &stats);
    thread t_process(&Process::run, &process);

    // Debug
    clients_to_add.push("1.2.3.4");

    // Aguarda encerramento do programa
    while(!t_discovery.joinable() && !t_process.joinable() && !t_discovery.joinable());
    t_discovery.join();
    t_process.join();
    t_discovery.join(); 

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
            continue; // Se já consta, não faz nada
        }

        // Adiciona à lista
        ClientData new_client(ip,STARTING_BALANCE,0); // Cria novo cliente
        mutex_client_list.lock();
        clients.insert({ip, new_client});
        stats.num_clients++;
        mutex_client_list.unlock();

    }
}