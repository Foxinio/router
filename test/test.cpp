#include <iostream>
#include <fcntl.h>
#include <unistd.h>

#include "../utils.h"
#include "../network_node.h"

int main() {
    while(true) {
        std::string s;
        std::getline(std::cin, s);
        auto [ip, mask] = inet::get_addr_with_mask(s);
//        inet::get_addr()
        std::cout << "ip:" << inet::get_addr(ip) << ",mask:" << (int)mask << std::endl;
        std::cout << "network:" << inet::get_addr(interface::get_network(ip, mask))
                  << ",broadcast:" << inet::get_addr(interface::get_broadcast(ip,mask)) << std::endl;
    }
}
