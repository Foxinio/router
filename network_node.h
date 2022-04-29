//
// Created by foxinio on 4/23/22.
//
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

class network_node {
public:
    uint32_t network_ip;
    uint32_t route_addr; // if neighbour route_addr is duplicate of addr;
    uint32_t unreachable_since = 0;
    uint32_t dist;
    uint8_t mask;
    bool connected_directly = false;
    network_node(uint32_t network_ip, uint32_t route_addr, uint8_t mask, uint32_t dist);
    network_node(uint32_t network_ip, uint8_t mask, uint32_t dist);
    std::string format() const;

    bool send_dist(int socket_fd, uint32_t ip, uint8_t outgoing_mask) const;

    void attempt_update(uint32_t new_route_addr, uint32_t dist_to_route, uint32_t new_dist, uint32_t turn);
    void update_dist(uint32_t new_route, uint32_t dist_to_route, uint32_t new_dist_from_route);

    static bool is_dist_inf(uint32_t dist);
    static uint32_t inf;

};


class interface {
public:
    uint32_t dist;
    uint32_t unreachable_since = 0;
    uint32_t broadcast_ip;
    uint32_t network_ip;
    uint8_t my_mask;
    bool reachable;

    interface(uint32_t ip, uint32_t dist, uint8_t mask, bool reachable);
    void mark_unreachable(uint32_t turn);
    void mark_reachable();
    static uint32_t get_broadcast(uint32_t ip, uint8_t mask);
    static uint32_t get_network(uint32_t ip, uint8_t mask);
};

using routing_table = std::vector<network_node>;
using interface_table = std::vector<interface>;

std::ostream& operator<<(std::ostream& out, const routing_table& tab);

class is_same_network {
    uint32_t my_ip;
    uint8_t my_mask;

public:
    is_same_network(uint32_t my_ip, uint8_t my_mask);
    bool operator()(const network_node& i) const;
    bool operator()(const interface& n) const;
};

template<typename T, typename U>
class is_member_equal {
    T val;
    T U::*ptr;
public:
    is_member_equal(T val, T U::*ptr)
        : val(val), ptr(ptr) {}
    bool operator()(U oth) {
        return oth.*ptr == val;
    }
};

