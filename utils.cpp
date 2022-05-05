//
// Created by foxinio on 4/22/22.
//

#include "utils.h"

#include <arpa/inet.h>

namespace inet {

not_valid_address_exception::not_valid_address_exception() {}

const char *not_valid_address_exception::what() const noexcept {
    return "given value doesn't represent valid IPv4 address.";
}



int find_zero(char* c, int size) {
    for(int i = 0; i < size; i++) {
        if(c[i] == 0) {
            return i;
        }
    }
    return -1;
}
std::string get_addr_with_mask(uint32_t addr, uint8_t mask) {
    char buffer[19] = {0};
    if (inet_ntop(AF_INET, &addr, buffer, 19) == nullptr) {
        switch(errno) {
            case EAFNOSUPPORT: throw std::runtime_error("idk what happened");
            case ENOSPC: throw not_valid_address_exception();
        }
    }
    int slash = find_zero(buffer, 19);
    if(slash == -1) {
        throw not_valid_address_exception();
    }
    buffer[slash] = '/';
    if(mask<10) {
        buffer[slash + 1] = (char)mask + '0';
    }
    else {
        buffer[slash + 1] = (char) (mask / 10) + '0';
        buffer[slash + 2] = (char) (mask % 10) + '0';
    }
    return buffer;
}

std::pair<uint32_t,uint8_t> get_addr_with_mask(const std::string& addr) {
    uint64_t slash = addr.find('/');
    if(slash == std::string::npos) {
        throw not_valid_address_exception();
    }
    uint8_t mask;
    if(addr.length() == slash+3) {
        mask = (addr[slash + 1] - '0') * 10 + addr[slash + 2] - '0';
    }
    else {
        mask = (addr[slash + 1] - '0');
    }
    uint32_t res = 0;
    char buffer[19] = {0};
    addr.copy(buffer, 19);
    buffer[slash] = 0;
    switch(inet_pton(AF_INET, buffer, &res)) {
        case 1: return {res, mask};
        case 0: throw not_valid_address_exception();
        default: throw std::runtime_error("idk what happened");
    }
}

std::string get_addr(uint32_t addr) {
    char buffer[15] = {0};
    if (inet_ntop(AF_INET, &addr, buffer, 19) == nullptr) {
        switch(errno) {
            case EAFNOSUPPORT: throw std::runtime_error("idk what happened");
            case ENOSPC: throw not_valid_address_exception();
        }
    }
    return buffer;
}

uint32_t get_addr(const std::string& addr) {
    uint32_t res;
    switch(inet_pton(AF_INET, addr.c_str(), &res)) {
        case 1: return res;
        case 0: throw not_valid_address_exception();
        default: throw std::runtime_error("idk what happened");
    }
}

std::string get_addr_str(uint32_t addr) {
    return get_addr(addr);
}
std::string get_addr_with_mask_str(uint32_t addr, uint8_t mask) {
    return get_addr_with_mask(addr, mask);
}

}
