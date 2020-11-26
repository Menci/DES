#include <iostream>
#include <cstring>
#include <cstdio>
#include <alloca.h>

#include "des.h"

void test_des_cbc(const char *input, uint64_t key) {
    size_t n = strlen(input) + 1;
    size_t output_length = des_cbc_output_length(n);

    char *encrypted = static_cast<char *>(alloca(output_length));
    des_cbc(0, key, input, n, encrypted, true);

    std::cerr << "Encrypted data: ";
    for (size_t i = 0; i < output_length; i++)
        printf("%2.2hhx%c", encrypted[i], " \n"[i == output_length - 1]);

    char *decrypted = static_cast<char *>(alloca(output_length));
    des_cbc(0, key, encrypted, output_length, decrypted, false);

    std::cerr << "Decrypted data: ";
    std::cout << decrypted << std::endl;
}

int main() {
    des_init();

    std::cerr << "Input the key (64-bit unsigned integer): ";
    uint64_t key;
    if (!(std::cin >> key)) {
        key = 0;
        std::cin.clear();
    }

    while (!std::cin.eof()) {
        std::cerr << "Input the data to be tested: ";

        std::string line;
        while (line.empty()) {
            if (std::cin.eof()) break;
            std::getline(std::cin, line);
        }

        test_des_cbc(line.c_str(), key);
    }

    return 0;
}
