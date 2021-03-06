// Szymon Jedras, i322920

#include "network_node.h"

#include "utils.h"
#include "dgram.h"

#include <string>
#include <iostream>

std::string network_node::format() const {
    std::string network = inet::get_addr_with_mask(network_ip, mask);
    std::string route = (connected_directly ? " connected directly" : " via " + inet::get_addr(route_addr));
    std::string dist_str = (!is_dist_inf(dist) ? " distance " + std::to_string(dist) : " unreachable");
    return  network + dist_str + route;
}

network_node::network_node(uint32_t network_ip, uint8_t mask, uint32_t route_addr, uint32_t route_mask, uint32_t dist)
    : network_ip(network_ip)
    , route_addr(route_addr)
    , dist(dist)
    , mask(mask)
    , route_mask(route_mask) {}

network_node::network_node(uint32_t network_ip, uint8_t mask, uint32_t dist)
    : network_node(network_ip, mask, network_ip, mask, dist) {
    connected_directly = true;
}

bool network_node::send_dist(int socket_fd, uint32_t outgoing_ip, uint8_t outgoing_mask, uint32_t turn) const {
   debug("sending dist to " << inet::get_addr_with_mask(outgoing_ip, outgoing_mask) << ", about node: "
   << "\n\t[ip:" << inet::get_addr_with_mask(network_ip,mask) << ",route:" << inet::get_addr_with_mask(route_addr, route_mask)
   << ",dist:" << dist << ",unreachable_since:" << unreachable_since << "]");
    long result = 9;
    if(((interface::get_network(outgoing_ip, outgoing_mask) == interface::get_network(route_addr, route_mask) &&
            interface::get_network(outgoing_ip, outgoing_mask) != interface::get_network(network_ip, mask)) ||
            (is_dist_inf(dist) && turn < unreachable_since + 5))) {
        debug("sending to on the path -> sending inf\n");
        result = dgram{network_ip, mask, (uint32_t) -1}.send(socket_fd, outgoing_ip);
    }
    else if(!is_dist_inf(dist)){
        debug("sending to on not the path -> sending " << dist << "\n");
        result = dgram{network_ip, mask, dist}.send(socket_fd, outgoing_ip);
    }
    return result == 9;
}

void network_node::attempt_update(uint32_t new_route_addr, uint8_t new_route_mask, uint32_t dist_to_route,
                                  uint32_t new_dist, uint32_t turn, bool connected_directly) {
    debug("attempting to update routing table, elem: [" << format() << "] new_addr: " << inet::get_addr_with_mask(new_route_addr, new_route_mask)
        << ", dist_to_route: " << dist_to_route << ", new_dist: " << new_dist << "\n");
    if(new_route_addr == route_addr) {
        debug("same route addr. ");
        if(is_dist_inf(new_dist)) {
            debug("updating to inf. ");
            if(!is_dist_inf(this->dist)) {
                debug("first message of this kind. ");
                unreachable_since = turn;
            }
            this->dist = inf;
        }
        else {
            debug("normal update/possible increase in consts.");
            this->dist = new_dist + dist_to_route;
        }
        debug("\n");
    }
    else if(!is_dist_inf(new_dist) && new_dist + dist_to_route < dist) {
        debug("normal update.\n");
        this->route_addr = new_route_addr;
        this->route_mask = new_route_mask;
        this->dist = new_dist + dist_to_route;
        this->connected_directly = connected_directly;
    }
}

bool network_node::is_dist_inf(uint32_t dist) {
    return dist >= 0xff;
}
uint32_t network_node::inf = (uint32_t)-1;

void network_node::set_unreachable(uint32_t turn) {
    if(!is_dist_inf(dist)) {
        dist = network_node::inf;
        unreachable_since = turn;
    }
}

interface::interface(uint32_t ip, uint32_t dist, uint8_t mask, bool reachable)
    : dist(dist)
    , broadcast_ip(get_broadcast(ip, mask))
    , network_ip(get_network(ip, mask))
    , my_ip(ip)
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

std::ostream& operator<<(std::ostream& out, const routing_table& tab) {
    for(const auto& item : tab) {
        out << item.format() << "\n";
    }
    return out;
}

is_same_network::is_same_network(uint32_t my_ip, uint8_t my_mask)
    : my_ip(my_ip), my_mask(my_mask) {}

bool is_same_network::operator()(const network_node &i) const {
    return my_ip == i.network_ip && my_mask == i.mask;
}

bool is_same_network::operator()(const interface &n) const {
    return interface::get_network(my_ip, n.my_mask) == n.network_ip;
}

int best_fit::score(const interface &i) {
    if(interface::get_network(my_ip, i.my_mask) != i.network_ip)
        return -1;
    return i.my_mask;
}

best_fit::best_fit(uint32_t ip) : my_ip(ip) {}

bool best_fit::operator()(const interface &curr_best, const interface &iter) {
    return score(curr_best) < score(iter);
}
