#include "Interface.h"
using namespace std;

bool ipv4IsValid(const std::string& ipAddress) {
    const std::string reg = "(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9]?[0-9])";
    
    const std::regex ipv4Regex("^" + reg + "\\." + reg + "\\." + reg + "\\." + reg + "$");

    return std::regex_match(ipAddress, ipv4Regex);
}

inline std::string current_time_format() {
    auto now = std::chrono::system_clock::now();
    auto time_type = std::chrono::system_clock::to_time_t(now);
    auto *local_time = std::localtime(&time_type);

    std::stringstream string_stream;
    string_stream << std::put_time(local_time, "%Y-%m-%d %H:%M:%S");

    return string_stream.str();
}

Command Interface::getCommand() {
    Command cmd;
    std::string addr;
    
    std::cin >> addr >> cmd.amount;

    if(!ipv4IsValid(addr)) {
        std::cerr << current_time_format() << " | Invalid IP address format. Try again." << std::endl;
        return getCommand();
    }
    if(cmd.amount < 0) {
        std::cerr << current_time_format() << " | Amount must be positive. Try again." << std::endl;
        return getCommand();
    }

    // convert string to in_addr (ip struct)
    inet_pton(AF_INET, addr.c_str(), &(cmd.dest));

    return cmd;
}

void Interface::printCommandResult() {
    int command_count = rr->seq_num;
    int new_balance = rr->value;
    std::string server_ip = inet_ntoa(this->server_addr);
    std::string dest_ip = inet_ntoa(this->current_command.dest);

    std::cout << "\n" << current_time_format() << " server " << server_ip << " id_req " << command_count;
    std::cout << " dest " << dest_ip << " value " << this->current_command.amount << " new_balance " << new_balance << std::endl;
}

void Interface::run() {
    // Inicialização
    string server_ip = inet_ntoa(this->server_addr);
    cout << current_time_format() << " server " << server_ip << endl;

    while (true) {
        // Obtem comando do usuario e envia ao servidor
        current_command = getCommand();
        executeCommand(current_command);

        while(rr->status < RR_OK); // Aguarda processamento
        
        // Imprime resultado da requisição
        printCommandResult();
        rr->status = RR_INVALID;
    }
}

void Interface::executeCommand(Command command) {
    // Manda para o Process para enviar pro servidor e aguarda resultado
    string dest_ip = inet_ntoa(command.dest);
    rr->destination = dest_ip;
    rr->value = command.amount;
    rr->status = RR_SEND;    
}