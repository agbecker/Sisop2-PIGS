#include "Interface.h"
#include "../Utils.h"

using namespace std;

bool ipv4IsValid(const std::string& ipAddress) {
    const std::string reg = "(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9]?[0-9])";
    
    const std::regex ipv4Regex("^" + reg + "\\." + reg + "\\." + reg + "\\." + reg + "$");

    return std::regex_match(ipAddress, ipv4Regex);
}

Command Interface::getCommand() {
    Command cmd;
    std::string addr;
    
    std::cin >> addr >> cmd.amount;
    
    if(std::cin.fail()) {
        std::cin.clear(); // limpa o estado de erro
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // limpa o buffer
        std::cerr << current_time_format() << " | Invalid input format. Try again." << "\n";
        return getCommand();
    }
    
    if(!ipv4IsValid(addr)) {
        std::cerr << current_time_format() << " | Invalid IP address format. Try again." << std::endl;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return getCommand();
    }
    if(cmd.amount < 0) {
        std::cerr << current_time_format() << " | Amount must be positive. Try again." << std::endl;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
        return getCommand();
    }

    // convert string to in_addr (ip struct)
    inet_pton(AF_INET, addr.c_str(), &(cmd.dest));

    return cmd;
}

void Interface::printCommandResult() {
    int command_count = rr->seq_num;
    int new_balance = rr->value;
    std::string server_ip = inet_ntoa(*(this->server_addr));
    std::string dest_ip = inet_ntoa(this->current_command.dest);

    if(rr->status == RR_OK) {
        std::cout << "\n" << current_time_format() << " server " << server_ip << " id_req " << command_count;
        std::cout << " dest " << dest_ip << " value " << this->current_command.amount << " new_balance " << new_balance << std::endl;
    }

    if(rr->status == RR_BALANCE) {
        std::cout << "\n" << current_time_format() << " Saldo insuficiente!" << " value " << this->current_command.amount << " balance " << new_balance << std::endl;
    }

    if(rr->status == RR_NOTONLIST) {
        std::cout << "\n" << current_time_format() << " Destinatário não encontrado! Verifique o IP." << std::endl;
    }
}

void Interface::printInfo() {
    // Inicialização (ao descobrir o server)
    string server_ip = inet_ntoa(*(this->server_addr));
    cout << current_time_format() << " server " << server_ip << endl;

    // Loop para resultados de requisições
    while(true) {
        while(rr->status < RR_OK); // Aguarda processamento
        // Imprime resultado da requisição
        printCommandResult();
        rr->status = RR_INVALID;
    }
}

void Interface::run() {
    // Cria thread de escrita
    thread t_print(&Interface::printInfo, this);
    rr->status = RR_INVALID;

    while (true) {
        // Obtem comando do usuario e envia ao servidor
        current_command = getCommand();
        executeCommand(current_command);
    }
}

void Interface::executeCommand(Command command) {
    // Manda para o Process para enviar pro servidor e aguarda resultado
    string dest_ip = inet_ntoa(command.dest);
    rr->destination = dest_ip;
    rr->value = command.amount;
    rr->status = RR_SEND;    
}
