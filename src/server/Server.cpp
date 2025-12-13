#include "Server.h"
using namespace std;
using json = nlohmann::json;
namespace fs = std::filesystem;

// Used to update data from the manager to new backups
void update_clients(string json_str) {
    try {
        json j = json::parse(json_str);
        if (j.contains("clients")) {
            mutex_client_list.lock();
            for (auto& element : j["clients"]) {
                string ip = element["ip"];
                int balance = element["balance"];
                int seq = element["seq"];
                
                if (clients.find(ip) == clients.end()) {
                    clients[ip] = ClientData(ip, balance, seq);
                } else {
                    clients[ip].balance = balance;
                    clients[ip].seq_num = seq;
                }
            }
            mutex_client_list.unlock();
        }
        if (j.contains("stats")) {
            stats.num_clients = j["stats"]["num_clients"];
            stats.total_transferred = j["stats"]["total_transferred"];
            stats.num_transactions = j["stats"]["num_transactions"];
        }
    } catch (exception& e) {
        // Ignore parse errors
    }
}

int main(int argc, char **argv) {
    // Obtém porta
    if(argc < 3) {
        cout << "Informe o número de porta e o ID do processo" << endl;
        return 1;
    }

    port = stoi(argv[1]);
    id = stoi(argv[2]);

    // Ingressa no multicast
    Multicast* multicast = new Multicast(id);
    multicast->init();

    // Verifica se há outro servidor já conectado
    multicast->find_others(&is_replica_manager);

    while(true){
        if(is_replica_manager) {
            multicast->start_manager();
            // roda enquanto for manager
            main_manager(multicast);
            //quando não for mais, troca de estado
            is_replica_manager = false;
        }
        else {
            multicast->start_backup();
            //mesma coisa aqui
            main_backup(multicast);
            is_replica_manager = true;
        }
    }

    return 0;
}

// Operações do Replica Manager quando ele vem ao poder
void main_manager(Multicast* multicast) {
    cout << "=== I AM THE MANAGER ===" << endl;

     // Impressão de inicialização
    const int num_transactions = stats.num_transactions;
    const long unsigned int total_transferred = stats.total_transferred;
    const long unsigned int total_balance = stats.num_clients * STARTING_BALANCE;
    
    cout << current_time_format() << " num_transactions " << num_transactions << " total_transferred " << total_transferred << " total_balance " << total_balance << endl;

    // Quando um novo backup surge, ele pode pedir o estado atual dos clientes
    // Usamos esse callback para isso
    multicast->set_on_state_request([](){ 
        mutex_client_list.lock();
        string s = serialize_server_data(&clients, &stats); 
        mutex_client_list.unlock();
        return s;
    });

    // Thread multicast para dar sinais de vida periódicos
    thread t_heartbeat(&Multicast::heartbeat, multicast);

    // Thread para ouvir mensagens de eleição de nós maiores
    thread t_listener(&Multicast::always_listening, multicast);

    // Thread para a interface do servidor
    initializeLogFile(transaction_history, TRANSACTION_HISTORY_FILEPATH);

    Interface interface(&events, &mtx_events, &transaction_history);
    thread t_interface(&Interface::run, &interface);

    // Thread para monitorar a adição de clientes à lista
    thread t_add_clients(add_clients);

    // Thread para descoberta de novos clientes
    Discovery discovery(clients_to_add, mutex_new_clients);
    vector<string> client_ips = list_client_ips();
    thread t_discovery(&Discovery::awaitRequest, &discovery);
    
    // Thread para informar aos clientes quem é o novo principal
    thread t_update_clients(&Discovery::update_clients_about_main, &discovery, client_ips);
    t_update_clients.detach();

    // Thread para processamento de requisições
    Process process(port, &clients, &mutex_client_list, &events, &mtx_events, &stats, multicast);
    thread t_process(&Process::run, &process);

    // Debug
    clients_to_add.push("1.2.3.4");

    // Aguarda encerramento do programa ou perda de liderança
    while(multicast->is_manager()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if(t_heartbeat.joinable()) t_heartbeat.join();
    if(t_listener.joinable()) t_listener.join();
    
    t_discovery.detach();
    t_process.detach();
    t_interface.detach();
    t_add_clients.detach();

    return;
}

// Operações de uma réplica
void main_backup(Multicast* multicast) {    
    cout << "=== I AM A BACKUP ===" << endl;
    multicast->set_on_update_received(update_clients);
    multicast->request_state();
    thread t_listener(&Multicast::always_listening, multicast);
    thread t_heart_doctor(&Multicast::monitor_rm_heartbeat, multicast);
    t_listener.join();
    t_heart_doctor.join();
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

// Cria lista com os IPs de todos os clientes
vector<std::string> list_client_ips() {
    mutex_client_list.lock();
    std::vector<std::string> ips;
    ips.reserve(clients.size());

    for (const auto& [_, data] : clients) {
        ips.push_back(data.ip);
    }
    mutex_client_list.unlock();

    return ips;
}