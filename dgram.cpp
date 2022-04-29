//
// Created by foxinio on 4/23/22.
//

#include "dgram.h"

#include "Init.h"
#include "utils.h"
#include "sys_wrappers.h"

dgram::dgram(uint32_t ip, uint8_t mask, uint32_t dist)
    : ip(ip), mask(mask), dist(dist) {}

std::pair<uint32_t,dgram> dgram::recv(int socket_fd) {
    dgram res;
    sockaddr_in sender{};
    socklen_t len;
    Recvfrom(socket_fd, &res, sizeof(dgram), 0, (sockaddr*)&sender, &len);
    debug("received packet raw [" << inet::get_addr_with_mask(res.ip, res.mask)
        << "," << std::hex << res.dist << "\n");
    res.ip = ntohl(res.ip);
    res.dist = ntohl(res.dist);
    debug("received packet inverted [" << inet::get_addr_with_mask(res.ip, res.mask)
        << "," << res.dist << "\n");
    return {sender.sin_addr.s_addr, res};
}

long dgram::send(int socket_fd) {
    sockaddr_in receiver = {
            .sin_family = AF_INET,
            .sin_port = PORT,
            .sin_addr = { .s_addr = (in_addr_t)this->ip },
            .sin_zero = {}
    };
    dgram to_send(htonl(ip), mask, htonl(dist));
    return Sendto(socket_fd, &to_send, sizeof(dgram), 0, (const sockaddr*)&receiver, sizeof(sockaddr_in));
}
