// Szymon Jedras, i322920


#include "dgram.h"

#include "Init.h"
#include "utils.h"
#include "sys_wrappers.h"

dgram::dgram(uint32_t network_ip, uint8_t mask, uint32_t dist)
    : network_ip(network_ip), mask(mask), dist(dist) {}

std::pair<uint32_t,dgram> dgram::recv(int socket_fd) {
    dgram res;
    sockaddr_in sender{};
    socklen_t len;
    Recvfrom(socket_fd, &res, sizeof(dgram), 0, (sockaddr*)&sender, &len);
//    debug("received packet raw [" << inet::get_addr_with_mask(res.network_ip, res.mask)
//        << ",0x" << std::hex << res.dist << std::dec << "]\n");
    res.network_ip = ntohl(res.network_ip);
    res.dist = ntohl(res.dist);
//    debug("received packet inverted [" << inet::get_addr_with_mask(res.network_ip, res.mask)
//        << "," << res.dist << "] from " << inet::get_addr(sender.sin_addr.s_addr) << "\n");
    return {sender.sin_addr.s_addr, res};
}

long dgram::send(int socket_fd, uint32_t broadcast) {
    sockaddr_in receiver = {
            .sin_family = AF_INET,
            .sin_port = PORT,
            .sin_addr = { .s_addr = (in_addr_t)broadcast },
            .sin_zero = {}
    };
    debug("sending packet raw [" << inet::get_addr_with_mask(network_ip, mask)
        << ",0x" << std::hex << dist << std::dec << "] to " << inet::get_addr(broadcast) << "\n");
    dgram to_send(htonl(network_ip), mask, htonl(dist));
//    debug("sent packet inverted [" << inet::get_addr_with_mask(to_send.network_ip, to_send.mask)
//        << "," << to_send.dist << "]\n");
    return Sendto(socket_fd, &to_send, sizeof(dgram), 0, (const sockaddr*)&receiver, sizeof(sockaddr_in));
}
