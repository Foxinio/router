//
// Created by foxinio on 5/4/22.
//

#include "sys_wrappers.h"

#include "dgram.h"
#include "utils.h"
#include "network_node.h"

#include <sys/socket.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>

#include <fstream>
#include <iostream>
#include <map>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"


static std::fstream test_out = std::fstream{ "test.out", std::ios::out };

int Bind(int fd, __CONST_SOCKADDR_ARG addr, socklen_t len) {
    test_out << "mock bind called with [" << fd << "," << addr << "," << len << "]\n\n";
    return 0;
}
int Socket(int domain, int type, int protocol) {
    test_out << "mock socket called with [" << domain << "," << type << "," << protocol << "]\n\n";
    return 3;
}

static const int ip172_16_0_0 = 4294906028;
static const int ip192_168_0_0 = 4278364352;
static const int ip10_0_0_0 = 4294967050;
enum sendto_ctlcodes {
    print,
    read_dgram,
    ip172_16_0_0_down = 2,
    ip192_168_0_0_down,
    ip10_0_0_0_down,
    ip172_16_0_0_up,
    ip192_168_0_0_up,
    ip10_0_0_0_up,
} poll_scenario[] = {
    read_dgram,         // 0
    read_dgram,
    read_dgram,
    read_dgram,
    read_dgram,
    // First turn done
    print,              // 5
    read_dgram,
    read_dgram,
    read_dgram,
    read_dgram,
    read_dgram,         // 10
    read_dgram,
    read_dgram,
    read_dgram,
    read_dgram,
    read_dgram,         // 15
    read_dgram,
    print,
    // Second turn done
    // testing distance degrading
    read_dgram,
    print,
    read_dgram,         // 20
    print,
    read_dgram,
    print,
    // testing connection losing on ip172_16_0_0
    read_dgram, // sending another connection on soon to disconnect interface
    //     first turn in this test
    print,              // 25
    read_dgram,
    read_dgram,
    read_dgram,
    read_dgram,
    read_dgram,         // 30
    print,
    print,
    print,
    //     second turn in this test
    read_dgram,
    read_dgram,         // 35
    read_dgram,
    read_dgram,
    read_dgram,
    print,
    print,              // 40
    print,
    print,
    print,
    // done
//     setting ip172_16_0_0 back up
//    read_dgram,
    // testing sendto returning error on ip10_0_0_0
    read_dgram,
    //     first turn
    ip10_0_0_0_down,    // 45
    read_dgram,
    read_dgram,
    read_dgram,
    read_dgram,
    read_dgram,         // 50
    print,
    print,
    print,
    print,
    // setting ip172_16_0_0 back up
    read_dgram,         // 55
    //     second turn
    read_dgram,
    read_dgram,
    print,
    print,
    print,              // 60
    print,
    // restart
    ip10_0_0_0_up,
    read_dgram,
    read_dgram,
    read_dgram,         // 65
    read_dgram,
    read_dgram,
    // First turn done
    print,
    read_dgram,
    read_dgram,         // 70
    read_dgram,
    read_dgram,
    read_dgram,
    read_dgram,
    read_dgram,         // 75
    read_dgram,
    read_dgram,
    read_dgram,
    read_dgram,
    print,              // 80
    // Second turn done
    read_dgram,
    print,
    };
std::map<uint32_t,long> sendto_resp = {
        {ip172_16_0_0, 9},
        {ip192_168_0_0, 9},
        {ip10_0_0_0, 9},
        };
std::map<sendto_ctlcodes,uint32_t> ctl_to_addr = {
        {ip172_16_0_0_down, ip172_16_0_0},
        {ip192_168_0_0_down, ip192_168_0_0},
        {ip10_0_0_0_down, ip10_0_0_0},
        {ip172_16_0_0_up, ip172_16_0_0},
        {ip192_168_0_0_up, ip192_168_0_0},
        {ip10_0_0_0_up, ip10_0_0_0},
        };
std::map<uint32_t,sendto_ctlcodes> addr_to_ctl = {
        {ip172_16_0_0, ip172_16_0_0_down},
        {ip192_168_0_0, ip192_168_0_0_down},
        {ip10_0_0_0, ip10_0_0_0_down},
        {ip172_16_0_0, ip172_16_0_0_up},
        {ip192_168_0_0, ip192_168_0_0_up},
        {ip10_0_0_0, ip10_0_0_0_up},
        };
int poll_step = 0;
int Poll(int sockfd, int timeout) {
    if(poll_step == sizeof poll_scenario / sizeof(int)) {
        std::cerr << "[EXITING] called mock poll " << poll_step << " times.\n";
        test_out << "[EXITING] called mock poll " << poll_step << " times.\n";
        std::exit(EXIT_SUCCESS);
    }
    test_out << "mock poll called [step:" << std::dec << poll_step << "], returning " << poll_scenario[poll_step] << "\n\n";
    sendto_ctlcodes res = poll_scenario[poll_step];
//    poll_step = (++poll_step)%(sizeof poll_scenario / sizeof(int));
    poll_step++;
    {
        switch (res) {
            case ip172_16_0_0_down:
                sendto_resp[ctl_to_addr[res]] = 0;
                res = (sendto_ctlcodes)Poll(sockfd, timeout);
                break;
            case ip192_168_0_0_down:
                sendto_resp[ctl_to_addr[res]] = 0;
                res = (sendto_ctlcodes)Poll(sockfd, timeout);
                break;
            case ip10_0_0_0_down:
                sendto_resp[ctl_to_addr[res]] = 0;
                res = (sendto_ctlcodes)Poll(sockfd, timeout);
                break;
            case ip172_16_0_0_up:
                sendto_resp[ctl_to_addr[res]] = 9;
                res = (sendto_ctlcodes)Poll(sockfd, timeout);
                break;
            case ip192_168_0_0_up:
                sendto_resp[ctl_to_addr[res]] = 9;
                res = (sendto_ctlcodes)Poll(sockfd, timeout);
                break;
            case ip10_0_0_0_up:
                sendto_resp[ctl_to_addr[res]] = 9;
                res = (sendto_ctlcodes)Poll(sockfd, timeout);
                break;
            default:
                break;
        }
    }
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
} recvfrom_scenario[] = {
        {"172.16.0.0/16", 2, "172.16.1.13"},
        {"192.168.2.0/24", 2, "192.168.2.5"},
        {"192.168.5.0/24", 2, "192.168.2.5"},
        {"10.0.0.0/8", 3, "10.0.1.1"},
        {"192.168.5.0/24", 2, "10.0.1.1"},
        // first turn done
        {"172.16.0.0/16", 2, "172.16.1.13"},                // 5
        {"192.168.2.0/24", (uint32_t)-1, "172.16.1.13"},
        {"10.0.0.0/8", (uint32_t)-1, "172.16.1.13"},
        {"192.168.2.0/24", 2, "192.168.2.5"},
        {"192.168.5.0/24", 2, "192.168.2.5"},
        {"172.16.0.0/16", (uint32_t)-1, "192.168.2.5"},     // 10
        {"10.0.0.0/8", (uint32_t)-1, "192.168.2.5"},
        {"10.0.0.0/8", 3, "10.0.1.1"},
        {"192.168.5.0/24", 2, "10.0.1.1"},
        {"192.168.2.0/24", 4, "10.0.1.1"},
        {"172.16.0.0/16", (uint32_t)-1, "192.168.2.5"},     // 15
        // second turn done
        // testing distance degrading
        {"12.16.0.0/16", 15, "172.16.1.13"},
        {"12.16.0.0/16", 4, "10.0.1.1"},
        {"12.16.0.0/16", 15, "10.0.1.1"},
        // testing connection losing on 172.16.1.13
        {"7.8.0.0/16", 1, "172.16.1.13"},
        //     first turn in this test
        {"12.16.0.0/16", 4, "10.0.1.1"},                    // 20
        {"192.168.2.0/24", 2, "192.168.2.5"},
        {"192.168.5.0/24", 2, "192.168.2.5"},
        {"10.0.0.0/8", 3, "10.0.1.1"},
        {"192.168.5.0/24", 2, "10.0.1.1"},
        //     second turn in this test
        {"12.16.0.0/16", 4, "10.0.1.1"},                    // 25
        {"192.168.2.0/24", 2, "192.168.2.5"},
        {"192.168.5.0/24", 2, "192.168.2.5"},
        {"10.0.0.0/8", 3, "10.0.1.1"},
        {"192.168.5.0/24", 2, "10.0.1.1"},
        // done
        // setting ip172_16_0_0 back up
//        {"172.16.0.0/16", 2, "172.16.1.13"},
        // testing sendto returning error on ip10_0_0_0
        {"12.16.0.0/16", (uint32_t)-1, "10.0.1.1"},               // 30
        //     first turn
        {"12.16.0.0/16", 4, "10.0.1.1"},
        {"192.168.2.0/24", 2, "192.168.2.5"},
        {"192.168.5.0/24", 2, "192.168.2.5"},
        {"10.0.0.0/8", 3, "10.0.1.1"},
        {"192.168.5.0/24", 2, "10.0.1.1"},                  // 35
        // setting ip172_16_0_0 back up
        {"172.16.0.0/16", 2, "172.16.1.13"},
        //     second turn
        {"192.168.2.0/24", 2, "192.168.2.5"},
        {"192.168.5.0/24", 2, "192.168.2.5"},
        // restart
        {"172.16.0.0/16", 2, "172.16.1.13"},
        {"192.168.2.0/24", 2, "192.168.2.5"},               // 40
        {"192.168.5.0/24", 2, "192.168.2.5"},
        {"10.0.0.0/8", 3, "10.0.1.1"},
        {"192.168.5.0/24", 2, "10.0.1.1"},
        // first turn done
        {"172.16.0.0/16", 2, "172.16.1.13"},
        {"192.168.2.0/24", (uint32_t)-1, "172.16.1.13"},    // 45
        {"10.0.0.0/8", (uint32_t)-1, "172.16.1.13"},
        {"192.168.2.0/24", 2, "192.168.2.5"},
        {"192.168.5.0/24", 2, "192.168.2.5"},
        {"172.16.0.0/16", (uint32_t)-1, "192.168.2.5"},
        {"10.0.0.0/8", (uint32_t)-1, "192.168.2.5"},        // 50
        {"10.0.0.0/8", 3, "10.0.1.1"},
        {"192.168.5.0/24", 2, "10.0.1.1"},
        {"192.168.2.0/24", 4, "10.0.1.1"},
        {"172.16.0.0/16", (uint32_t)-1, "192.168.2.5"},
        // second turn done
        {"192.168.5.0/24", (uint32_t)-1, "192.168.2.5"},    // 55
        };
int recvfrom_step = 0;
long Recvfrom(int fd, void *buf, size_t n, int flags, sockaddr *addr, socklen_t *addr_len) {

    if(recvfrom_step == sizeof recvfrom_scenario / sizeof(recvfromresp)) {
        std::cerr << "[EXITING] called mock recvfrom " << recvfrom_step << " times.\n";
        std::exit(EXIT_SUCCESS);
    }
    memcpy(buf, &recvfrom_scenario[recvfrom_step].buf, sizeof(dgram));
    ((sockaddr_in*)addr)->sin_addr.s_addr = recvfrom_scenario[recvfrom_step].sender;
    int res = recvfrom_scenario[recvfrom_step].result;
    test_out << "mock recvfrom called [step:" << recvfrom_step << "], returning data inverted ["
             << inet::get_addr_with_mask(ntohl(recvfrom_scenario[recvfrom_step].buf.network_ip), recvfrom_scenario[recvfrom_step].buf.mask)
             << ",0x" << std::hex << ntohl(recvfrom_scenario[recvfrom_step].buf.dist) << "], from "
             << inet::get_addr(recvfrom_scenario[recvfrom_step].sender) << "\n\n";
//    recvfrom_step = (++recvfrom_step)%(sizeof recvfrom_scenario / sizeof(recvfromresp));
    recvfrom_step++;
    return res;
}

long Sendto(int fd, const void *buf, size_t n, int flags, const sockaddr *addr, socklen_t addr_len) {
    dgram* buffer = (dgram*)buf;
    test_out << "mock sendto called with inverted data: ["
             << inet::get_addr_with_mask(htonl(buffer->network_ip), buffer->mask) << ",0x"
             << std::hex << htonl(buffer->dist) << "] to " << inet::get_addr(((sockaddr_in*)addr)->sin_addr.s_addr)
             << ", returning: " << sendto_resp[((sockaddr_in*)addr)->sin_addr.s_addr] << "\n";
    return sendto_resp[((sockaddr_in*)addr)->sin_addr.s_addr];
}

int Setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen) {
    test_out << "mock setsockopt\n\n";
    return 0;
}


#pragma GCC diagnostic pop
