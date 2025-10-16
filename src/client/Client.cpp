#include "Client.h"

using namespace std;

int main() {
    struct sockaddr_in serv_addr;
    if(!discoverServer(serv_addr)) {
        cout <<"Erro na conexão com o servidor" << endl;
        return 1;
    }

    cout << "Agora sei que o servidor tem IP " << inet_ntoa(serv_addr.sin_addr) << " para todo o sempre" << endl;

    // Inicia thread de Interface
    Interface interface(serv_addr.sin_addr);
    thread t_interface(&Interface::run, &interface);
    if (t_interface.joinable())
        t_interface.join();

    return 0;
}