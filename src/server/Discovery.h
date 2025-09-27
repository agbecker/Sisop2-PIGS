#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <iostream>
#include <chrono>
#include <thread>

class Discovery {
    private:
        std::string *str_pointer;

    public:
        void set_str(std::string* str);
        void run();
};



#endif