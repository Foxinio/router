//
// Created by foxinio on 4/23/22.
//

#include "dgram.h"

#include "Init.h"
#include "utils.h"
#include "sys_wrappers.h"

long get_dgram_len(int socket_fd) {
    char c;
    return Recvfrom(socket_fd, &c, 0, MSG_TRUNC | MSG_PEEK, NULL, 0);
}

std::pair<uint32_t,dgram> dgram::recv(int socket_fd) {
    dgram res;
    sockaddr_in sender{};
    socklen_t len;
    Recvfrom(socket_fd, &res, sizeof(dgram), 0, (sockaddr*)&sender, &len);
    res.ip = ntohl(res.ip);
    res.dist = ntohl(res.dist);
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

dgram::dgram(uint32_t ip, uint8_t mask, uint32_t dist)
    : ip(ip), mask(mask), dist(dist) {
}



