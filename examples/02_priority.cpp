#include <simply/concurrency.h>
#include <iostream>
#include <string>

void set_value(double& val_out, double val_in) {
    simply::this_thread::sleep(1000);
    val_out = val_in;
}

int main() {
    double value = 0;

    simply::Thread t({
        .priority = simply::Thread::Priority::LOWEST
    }, set_value, std::ref(value), 5);

    while ( !t.join(100) ) {
        std::cout << "Still waiting... Value is now: " << std::to_string(value) << std::endl;
    }

    std::cout << "Thread has now successfully joined!" << std::endl;
    std::cout << "Ended with value: " << std::to_string(value) << std::endl;
}