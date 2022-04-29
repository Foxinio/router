//
// Created by foxinio on 4/23/22.
//

#pragma once

#include <vector>

class dgram {
    dgram() = default;
public:

    dgram(uint32_t ip, uint8_t mask, uint32_t dist);

    uint32_t ip{};
    uint8_t mask{};
    uint32_t dist{};

    static std::pair<uint32_t,dgram> recv(int socket_fd);
    long send(int socket_fd);

} __attribute__((packed));

