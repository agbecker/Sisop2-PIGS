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

void Interface::show_stats(bool duplicate) {
    // Mostra dados do servidor

    cout << "num_transactions " << num_transactions;
    // Quebra de linha se não for duplicado
    if(duplicate) {
        cout << " ";
    }
    else {
        cout << endl;
    }

    cout << "total_transferred " << total_transferred << " total_balance " << total_balance << endl << endl;
    
}

void Interface::display_event(Event event) {
    cout << current_time_format() << " client " << event.origin;

    if(event.duplicate) {
        cout << " DUP!! ";
    }

    cout << " id_req " << event.seq_num << " dest " << event.destination << " value " << event.value << endl;
    show_stats(event.duplicate);
}

void Interface::run() {
    // Impressão de inicialização
    cout << current_time_format() << " num_transactions " << num_transactions << " total_transferred " << total_transferred << " total_balance " << total_balance << endl;

    while(true) {
        while(events->empty()); // Aguarda ocorrer um evento

        // Acessa a fila e imprime
        mtx_event->lock();
        Event event = events->front();
        events->pop();
        mtx_event->unlock();

        display_event(event);
    }
}