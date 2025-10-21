#include "Client.h"

using namespace std;

int main() {
    // Descoberta do servidor
    struct sockaddr_in serv_addr;
    while(!discoverServer(serv_addr));

    // Define o RequestReply para comunicação entre threads
    RequestReply rr;

    // Inicia thread de Interface
    Interface interface(serv_addr.sin_addr, &rr);
    thread t_interface(&Interface::run, &interface);

    // Inicia thread de Process
    Process process(serv_addr.sin_addr, &rr);
    thread t_process(&Process::run, &process);

    while(!t_interface.joinable() and !t_process.joinable());
    t_interface.join();
    t_process.join();

    return 0;
}