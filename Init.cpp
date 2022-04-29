//
// Created by foxinio on 4/23/22.
//

#include "Init.h"

#include "utils.h"
#include "sys_wrappers.h"

#include <arpa/inet.h>
#include <unistd.h>

Init::Init(int argc_, char* argv_[]) {
    argc = argc_;
    argv = argv_;
    handle_init();
}

interface_table Init::read_input() {
//    debug(std::cerr << "reading input.\n");
    int no_neighbours = 0;
    std::cin >> no_neighbours;
    interface_table res;
    for(int i = 0; i < no_neighbours; i++) {
        std::string addr_string;
        uint32_t dist;
        std::cin >> addr_string;
        std::cin.ignore(9);
        std::cin >> dist;
        auto [addr, mask] = inet::get_addr_with_mask(addr_string);
        res.emplace_back(addr, dist, mask, true);
//        debug(std::cerr << "read " << addr_string << " with dist: " << dist << "\n");
    }
    return res;
}

void Init::handle_init() {
    if(argc > 1) {
        std::cout << "Program: " << argv[0] << " doesn't expect any arguments";
        std::exit(EXIT_FAILURE);
    }
    interfaces = read_input();
    socket_fd = Socket(AF_INET, SOCK_DGRAM, 0);

//    sockaddr_in servaddr = {
//            .sin_family = AF_INET,
//            .sin_port = PORT,
//            .sin_addr = { .s_addr = INADDR_ANY } ,
//            .sin_zero = {}
//    };
//    Bind(socket_fd, (const sockaddr *)&servaddr, sizeof(sockaddr_in));
    int broadcastPermission = 1;
    Setsockopt(socket_fd, SOL_SOCKET, SO_BROADCAST, &broadcastPermission, sizeof(int));
}

int Init::get_socket_fd() const {
    return socket_fd;
}

interface_table Init::get_interfaces() {
    return interfaces;
}

Init::~Init() {
    close(socket_fd);
}
