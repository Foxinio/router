//
// Created by foxinio on 4/23/22.
//

#include "network_node.h"

#include "utils.h"
#include "dgram.h"

std::string network_node::format() const {
    return inet::get_addr_with_mask(network_ip, mask)
        + (!is_dist_inf(dist) ? " distance " + std::to_string(dist) : " unreachable")
        + (connected_directly ? " connected directly" : " via " + inet::get_addr(route_addr));
}

network_node::network_node(uint32_t network_ip, uint32_t route_addr, uint8_t mask, uint32_t dist)
    : network_ip(network_ip)
    , route_addr(route_addr)
    , unreachable_since(-1)
    , dist(dist)
    , mask(mask) {}

network_node::network_node(uint32_t network_ip, uint8_t mask, uint32_t dist)
    : network_node(network_ip, network_ip, mask, dist) {
    connected_directly = true;
}

bool network_node::send_dist(int socket_fd, uint32_t ip) const {
    if(connected_directly)
        return dgram{ip, mask, (uint32_t)-1}.send(socket_fd) >= 0;
    return dgram{ip, mask, dist}.send(socket_fd) >= 0;
}

void network_node::attempt_update(uint32_t new_route_addr, uint32_t dist_to_route, uint32_t new_dist, uint32_t turn) {
    if(new_route_addr == route_addr) {
        if(is_dist_inf(new_dist)) {
            if(!is_dist_inf(this->dist)) {
                unreachable_since = turn;
            }
            this->dist = new_dist;
        }
        else {
            this->dist = new_dist + dist_to_route;
        }
    }
    else if(!is_dist_inf(new_dist) && new_dist + dist_to_route < dist) {
        this->route_addr = new_route_addr;
        this->dist = new_dist + dist_to_route;
    }
}

bool network_node::is_dist_inf(uint32_t dist) {
    return dist >= inf;
}
uint32_t network_node::inf = 0xffff;

interface::interface(uint32_t ip, uint32_t dist, uint8_t mask, bool reachable)
    : dist(dist)
    , broadcast_ip(get_broadcast(ip, mask))
    , network_ip(get_network(ip, mask))
    , my_mask(mask)
    , reachable(reachable) {}

void interface::mark_unreachable(uint32_t turn) {
    reachable = false;
    unreachable_since = turn;
}

void interface::mark_reachable() {
    reachable = true;
}

uint32_t interface::get_broadcast(uint32_t ip, uint8_t mask) {
    return ip | ((uint32_t)-1)<<(mask);
}

uint32_t interface::get_network(uint32_t ip, uint8_t mask) {
    return ip & ((uint32_t)-1)>>(32-mask);
}

is_same_network::is_same_network(uint32_t my_ip, uint8_t my_mask)
    : my_ip(my_ip), my_mask(my_mask) {}

bool is_same_network::operator()(const network_node &i) const {
    return my_ip == i.network_ip && my_mask == i.mask;
}

bool is_same_network::operator()(const interface &n) const {
    return interface::get_network(my_ip, n.my_mask) == n.network_ip;
}
