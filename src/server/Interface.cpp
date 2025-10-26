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

void Interface::show_stats(bool duplicate, ServerStats stats) {
    // Mostra dados do servidor

    cout << "num_transactions " << stats.num_transactions;
    // Quebra de linha se não for duplicado
    if(duplicate) {
        cout << " ";
    }
    else {
        cout << endl;
    }

    cout << "total_transferred " << stats.total_transferred << " total_balance " << stats.num_clients*STARTING_BALANCE << endl << endl;
    
}

void Interface::display_event(Event event) {
    cout << current_time_format() << " client " << event.origin;

    if(event.duplicate) {
        cout << " DUP!! ";
    }

    cout << " id_req " << event.seq_num << " dest " << event.destination << " value " << event.value << endl;
    show_stats(event.duplicate, event.stats);
}

void Interface::register_event(Event event) {
    *transaction_history << current_time_format() << " client " << event.origin;
    *transaction_history << " id_req " << event.seq_num << " dest " << event.destination << " value " << event.value << "\n";
    *transaction_history << "num_transactions " << event.stats.num_transactions;
    *transaction_history << "\n";
    *transaction_history << "total_transferred " << event.stats.total_transferred << " total_balance " << event.stats.num_clients*STARTING_BALANCE << "\n" << "\n";
    transaction_history->flush();
}

void Interface::run() {
    // Impressão de inicialização
    cout << current_time_format() << " num_transactions " << 0 << " total_transferred " << 0 << " total_balance " << 0 << endl;

    while(true) {
        while(events->empty()); // Aguarda ocorrer um evento

        // Acessa a fila e imprime
        mtx_event->lock();
        Event event = events->front();
        events->pop();
        mtx_event->unlock();

        display_event(event);

        if (!event.duplicate) {
            register_event(event);
        }   
    }
}

