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

dgram dgram::recv(int socket_fd) {
    long dgram_size = get_dgram_len(socket_fd);
    uint8_t* buffer = new uint8_t[dgram_size+1];
    sockaddr_in sender{};
    socklen_t len;
    dgram_size = Recvfrom(socket_fd, buffer, dgram_size, 0, (sockaddr*)&sender, &len);
    buffer[dgram_size] = 0;
    dgram res = { .ip = (int)sender.sin_addr.s_addr, .payload = {buffer, buffer+dgram_size}};
    delete[] buffer;
    return res;
}

long dgram::send(int socket_fd) {
    sockaddr_in receiver = {
            .sin_family = AF_INET,
            .sin_port = PORT,
            .sin_addr = { .s_addr = (in_addr_t)ip },
            .sin_zero = {}
    };
    return Sendto(socket_fd, payload, payload.length(), 0, (const sockaddr*)&receiver, sizeof(sockaddr_in));
}

