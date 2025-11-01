#include "concurrency.hpp"
#include <iostream>

void hello_world() {
    std::cout << "Hello World (from thread)!" << std::endl;
}

int main() {
    simply::Thread t(hello_world);
}
