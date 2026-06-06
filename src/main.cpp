#include <cstring>
#include <iostream>

#include "common_header.hpp"

int main(int ac, char **av) {
    if (ac < 2)
        return (1);
    CommonHeader h;
    if (CommonHeader::try_from_string((uint8_t *)av[1], strlen(av[1]), h))
        std::cout << h.as_cstr() << "\n";
    else
        std::cout << "Invalid Header\n";
}