#include "Server.h"
using namespace std;
namespace fs = std::filesystem;

int main(int argc, char **argv) {
    // Obtém porta
    if(argc < 3) {
        cout << "Informe o número de porta e o ID do processo" << endl;
        return 1;
    }

    int port = stoi(argv[1]);
    id = stoi(argv[2]);

    // Ingressa no multicast
    open_multicast();

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

// Ingressa no grupo multicast de comunicação entre as réplicas
void open_multicast() {
    socket_multicast = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_multicast < 0) {
        perror("socket");
        return;
    }

    int reuse = 1;
    setsockopt(socket_multicast, SOL_SOCKET, SO_REUSEADDR,
               &reuse, sizeof(reuse));

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = htons(12345);
    local.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(socket_multicast,
             (sockaddr*)&local, sizeof(local)) < 0) {
        perror("bind");
        return;
    }

    ip_mreq mreq{};
    inet_pton(AF_INET, "239.0.0.1", &mreq.imr_multiaddr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(socket_multicast, IPPROTO_IP,
                   IP_ADD_MEMBERSHIP,
                   &mreq, sizeof(mreq)) < 0) {
        perror("IP_ADD_MEMBERSHIP");
        return;
    }
}