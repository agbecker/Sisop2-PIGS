#include "Server.h"
using namespace std;
namespace fs = std::filesystem;

int main(int argc, char **argv) {
    // Obtém porta
    if(argc < 3) {
        cout << "Informe o número de porta e o ID do processo" << endl;
        return 1;
    }

    port = stoi(argv[1]);
    id = stoi(argv[2]);

    // Ingressa no multicast
    Multicast* multicast = new Multicast();
    multicast->init();

    // Verifica se há outro servidor já conectado
    multicast->find_others(&is_replica_manager);

    if(is_replica_manager) {
        main_manager(multicast);
    }

    else {
        main_backup(multicast);
    }

    return 0;
}

// Operações do Replica Manager quando ele vem ao poder
void main_manager(Multicast* multicast) {
    // Thread de multicast para acolher novas réplicas
    thread t_replica_discovery(&Multicast::welcome_new_replicas, multicast);

    // Thread multicast para dar sinais de vida periódicos
    thread t_heartbeat(&Multicast::heartbeat, multicast);

    // Thread para a interface do servidor
    initializeLogFile(transaction_history, TRANSACTION_HISTORY_FILEPATH);

    Interface interface(&events, &mtx_events, &transaction_history);
    thread t_interface(&Interface::run, &interface);

    // Thread para monitorar a adição de clientes à lista
    thread t_add_clients(add_clients);

    // Thread para descoberta de novos clientes
    Discovery discovery(clients_to_add, mutex_new_clients);
    thread t_discovery(&Discovery::awaitRequest, &discovery);

    // Thread para processamento de requisições
    Process process(port, &clients, &mutex_client_list, &events, &mtx_events, &stats);
    thread t_process(&Process::run, &process);

    // Debug
    // clients_to_add.push("1.2.3.4");

    // Aguarda encerramento do programa
    while(!t_discovery.joinable() && !t_process.joinable() && !t_discovery.joinable());
    t_discovery.join();
    t_process.join();
    t_discovery.join(); 

    return;
}

// Operações de uma réplica
void main_backup(Multicast* multicast) {
    cout << "As passivas reinam" << endl;
    while(true);
    return;
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



// inicializa handler de arquivos - cria caminho esperado caso nao exista
void initializeLogFile(std::fstream& handler, const std::string& logPath) {
    const std::string PATH = logPath;
    
    // Primeira tentativa de abertura
    handler.open(PATH, std::ios::in | std::ios::out | std::ios::trunc);
    
    if (!handler.is_open()) {
        // std::cout << "Arquivo não encontrado. Criando caminho esperado..." << std::endl;
        
        try {
            // Extrai o diretório do caminho
            fs::path pathObj(PATH);
            fs::path directory = pathObj.parent_path();
            
            // Cria o diretório se não existir e for necessário
            if (!directory.empty() && !fs::exists(directory)) {
                if (fs::create_directories(directory)) {
                    // std::cout << "Diretório criado: " << directory << std::endl;
                }
            }
            
            handler.open(PATH, std::ios::in | std::ios::out | std::ios::trunc);
            if (handler.is_open()) {
                // std::cout << "Arquivo de log criado com sucesso: " << PATH << std::endl;
            } else {
                std::cerr << "Falha ao criar arquivo após criar diretórios: " << PATH << std::endl;
            }
            
        } catch (const fs::filesystem_error& ex) {
            std::cerr << "Erro no filesystem: " << ex.what() << std::endl;
        }
    } else {
        // std::cout << "Arquivo de log aberto com sucesso: " << PATH << std::endl;
    }
}

