#include "Server.h"
using namespace std;

int main() {
    // Interface interface;
    // Discovery discovery;

    // thread t_interface(&Interface::run, &interface);

    // string str = "Gayzinhos se amando";
    // discovery.set_str(&str);
    // thread t_discovery(&Discovery::run, &discovery);
    // this_thread::sleep_for(chrono::seconds(1));
    // str = "Gayzinhos não se amam mais";


    // while(true) {
    //     string command;
    //     cin >> command;

    //     if (command == "exit" or command == "quit") {
    //         break;
    //     }
    // }

    // t_interface.join();
    // t_discovery.join();


    Discovery discovery;
    discovery.awaitRequest();

    return 0;
}