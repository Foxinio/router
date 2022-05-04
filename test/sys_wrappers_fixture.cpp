//
// Created by foxinio on 5/4/22.
//

#include "../sys_wrappers.h"

#include "../dgram.h"
#include "../utils.h"

#include <sys/socket.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <fstream>
#include <iostream>

static std::fstream test_out = std::fstream{ "test.out", std::ios::out };

int Bind(int fd, __CONST_SOCKADDR_ARG addr, socklen_t len) {
    test_out << "mock bind called\n\n";
    return 0;
}
int Socket(int domain, int type, int protocol) {
    test_out << "mock socket called\n\n";
    return 3;
}


int Poll(int sockfd, int timeout) {
    static int responces[] = {
//        0,
        1,
//        0,
        1,
        1,
//        0,
        1,
        1,
        0,
        1,
        1,
        1,
//        0,
        1,
        1,
        1,
        1,
//        0,
        1,
        1,
        1,
        1,
        0,
    };
    static int index = 0;
    if(index == sizeof responces / sizeof(int)) {
        std::cerr << "[EXITING] called mock poll " << index << " times.\n";
        std::exit(EXIT_SUCCESS);
    }
    test_out << "mock poll called returning " << responces[index] << "\n\n";
    int res = responces[index];
//    index = (++index)%(sizeof responces / sizeof(int));
    index++;
    return res;
}

struct recvfromresp {
    dgram buf;
    in_addr_t sender;
    int result;
    recvfromresp(int result)
        : buf(0, 0, 0), sender(0), result(result) {}
    recvfromresp(uint32_t network, uint8_t mask, uint32_t dist, in_addr_t sender, int result)
        : buf(htonl(network), mask, htonl(dist)), sender(sender), result(result) {}
    recvfromresp(std::pair<uint32_t, uint8_t> addr, uint32_t dist, in_addr_t sender, int result)
        : recvfromresp(addr.first, addr.second, dist, sender, result) {}
    recvfromresp(std::string network, uint32_t dist, std::string sender, int result)
        : recvfromresp(inet::get_addr_with_mask(network), dist, inet::get_addr(sender), result) {}
    recvfromresp(std::string network, uint32_t dist, std::string sender)
        : recvfromresp(network, dist, sender, 9) {}
    recvfromresp(std::string network, uint8_t mask, uint32_t dist, std::string sender, int result)
        : recvfromresp(inet::get_addr(network), mask, dist, inet::get_addr(sender), result) {}
};
long Recvfrom(int fd, void *buf, size_t n, int flags, sockaddr *addr, socklen_t *addr_len) {
    static recvfromresp responces[] = {
            {"172.16.0.0/16", 2, "172.16.1.13"},
            {"192.168.2.0/24", 2, "192.168.2.5"},
            {"192.168.5.0/24", 2, "192.168.2.5"},
            {"10.0.0.0/8", 3, "10.0.1.1"},
            {"192.168.5.0/24", 2, "10.0.1.1"},
            // first turn done
            {"172.16.0.0/16", 2, "172.16.1.13"},
            {"192.168.2.0/24", (uint32_t)-1, "172.16.1.13"},
            {"10.0.0.0/8", (uint32_t)-1, "172.16.1.13"},
            {"192.168.2.0/24", 2, "192.168.2.5"},
            {"192.168.5.0/24", 2, "192.168.2.5"},
            {"172.16.0.0/16", (uint32_t)-1, "192.168.2.5"},
            {"10.0.0.0/8", (uint32_t)-1, "192.168.2.5"},
            {"10.0.0.0/8", 3, "10.0.1.1"},
            {"192.168.5.0/24", 2, "10.0.1.1"},
            {"192.168.2.0/24", 4, "10.0.1.1"},
            {"172.16.0.0/16", (uint32_t)-1, "192.168.2.5"},
            // second turn done
    };
    static int index = 0;
    if(index == sizeof responces / sizeof(recvfromresp)) {
        std::cerr << "[EXITING] called mock recvfrom " << index << " times.\n";
        std::exit(EXIT_SUCCESS);
    }
    test_out << "mock recvfrom called, returning data raw ["
             << inet::get_addr_with_mask(responces[index].buf.network_ip, responces[index].buf.mask)
             << ",0x" << std::hex << responces[index].buf.dist << "], from "
             << inet::get_addr(responces[index].sender) << "\n";
    memcpy(buf, &responces[index].buf, sizeof(dgram));
    ((sockaddr_in*)addr)->sin_addr.s_addr = responces[index].sender;
    int res = responces[index].result;
    test_out << "mock recvfrom called, returning data inverted ["
             << inet::get_addr_with_mask(ntohl(responces[index].buf.network_ip), responces[index].buf.mask)
             << ",0x" << std::hex << ntohl(responces[index].buf.dist) << "], from "
             << inet::get_addr(responces[index].sender) << "\n\n";
//    index = (++index)%(sizeof responces / sizeof(recvfromresp));
    index++;
    return res;
}

long Sendto(int fd, const void *buf, size_t n, int flags, const sockaddr *addr, socklen_t addr_len) {
    dgram* buffer = (dgram*)buf;
    test_out << "mock sendto called with raw data: "
             << inet::get_addr_with_mask(buffer->network_ip, buffer->mask) << ",0x"
             << std::hex << buffer->dist << "] to " << inet::get_addr(((sockaddr_in*)addr)->sin_addr.s_addr) << "\n";
    dgram unpacked{ ntohl(buffer->network_ip), buffer->mask, ntohl(buffer->dist) };
    test_out << "mock sendto called with inverted data [" << inet::get_addr_with_mask(unpacked.network_ip, unpacked.mask)
        << "," << unpacked.dist << "]\n\n";
    return 0;
}

int Setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen) {
    test_out << "mock setsockopt called\n\n";
    return 0;
}
