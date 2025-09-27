#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <map>

class Server {
    private:
        std::map<std::string, int> clients; // Where the client's IP is the key, and the value is their balance
    
};

int main() {
    Server server;
    
}








#endif