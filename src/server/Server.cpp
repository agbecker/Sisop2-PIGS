#include <iostream>
#include "Interface.h"
#include <thread>
using namespace std;

int main() {
    Interface interface;

    thread t_interface(&Interface::run, &interface);

    t_interface.join();
    
    return 0;
}