#include "Discovery.h"
using namespace std;

void Discovery::set_str(string* str) {
    this->str_pointer = str;
}

void Discovery::run() {
    cout << *(this->str_pointer) << endl;
    this_thread::sleep_for(chrono::seconds(1));
    cout << *(this->str_pointer) << endl;
}