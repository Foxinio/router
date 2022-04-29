//
// Created by foxinio on 4/23/22.
//

#ifndef PRACOWNIA_2_DGRAM_H
#define PRACOWNIA_2_DGRAM_H

#include <vector>

class dgram {
public:
    int ip;
    std::vector<uint8_t> payload;

    static dgram recv(int socket_fd);
    long send(int socket_fd);
};
#endif //PRACOWNIA_2_DGRAM_H
