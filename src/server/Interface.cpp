#include "Interface.h"
using namespace std;

inline std::string current_time_format() {
    auto now = std::chrono::system_clock::now();
    auto time_type = std::chrono::system_clock::to_time_t(now);
    auto *local_time = std::localtime(&time_type);

    std::stringstream string_stream;
    string_stream << std::put_time(local_time, "%Y-%m-%d %H:%M:%S");

    return string_stream.str();
}

void show_stats() {
    
}

void Interface::run() {
    // Impressão de inicialização
    cout << current_time_format() << " num_transactions " << num_transactions << " total_transferred " << total_transferred << " total_balance " << total_balance << endl;

    
}