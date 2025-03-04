#include <stdexcept>
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <thread>
#include <chrono>

typedef std::vector<int> vec;

void 	clear() {
	std::cout << "\r\033[K";
}

void printFormattedFirst(int n) {
	std::cout << "\033[2;0H";
    for (int i = 0; i < n; ++i) {
				std::cout << "Current value: "
				<< i << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        clear();
    }
    std::cout << std::flush;
}

void printFormattedSecond(int n) {
	std::cout << "\033[3;0H";
    for (int i = 0; i < n; ++i) {
				std::cout << "Current value: "
				<< i << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        clear();
    }
    std::cout << std::flush;
}

int main(int ac, char **av) {

    int n = atoi(av[ac - 1]);

	printFormattedFirst(n);
	printFormattedSecond(n);
	std::cout << "TEST\n";
}