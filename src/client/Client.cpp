#include "Client.h"

using namespace std;

int main() {
    struct sockaddr_in serv_addr;
    if(!discoverServer(serv_addr)) {
        cout <<"Erro na conexão com o servidor" << endl;
        return 1;
    }

    cout << "Agora sei que o servidor tem IP " << inet_ntoa(serv_addr.sin_addr) << " para todo o sempre" << endl;

    return 0;
}