#include "utils.h"

#include "Init.h"
#include "dgram.h"
#include "sys_wrappers.h"

#include <iostream>
#include <utility>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <cstring>

using namespace std::chrono;
using namespace std::chrono_literals;

class router {
    interface_table interfaces;
    routing_table routing;
    int socket_fd;

    uint32_t turn;

    const seconds timeout = 30s;

public:
    router(interface_table&& interfaces, int socket_fd)
        : interfaces(std::move(interfaces))
        , socket_fd(socket_fd)
        , turn(0) {
        for(auto item : this->interfaces) {
            routing.emplace_back(item.network_ip, item.my_mask, item.dist);
        }
        debug("coppied interfaces to routing_table: " << routing);
    }

    int run() {
        debug("starting running\n");
        auto last_sent = time_point_cast<milliseconds>(high_resolution_clock::now()-timeout);
        while(true) {
            auto duration = duration_cast<milliseconds>(last_sent + timeout - high_resolution_clock::now());

            debug("\nentering Poll with wait time: " << std::max((int)duration.count(), 0) << "ms - ");
            if(Poll(socket_fd, std::max((int)duration.count(), 0)) == 0) {
                debug("\ttime's up, distributing table\n");
                distribute_table();
                clear_unreachable();
                last_sent = time_point_cast<milliseconds>(high_resolution_clock::now());
                turn++;
            }
            else {
                debug("\treceived datagram\n");
                read_table();
            }
        }
    }

    std::vector<network_node>::iterator find(uint32_t ip, uint8_t mask) {
        return std::find_if(routing.begin(), routing.end(), is_same_network(ip, mask));
    }

    std::vector<network_node>::iterator find_or_insert(uint32_t ip, uint8_t mask, uint32_t route_ip, uint8_t route_mask) {
//        debug("searching routing_table for " << inet::get_addr_with_mask(ip, mask) << "\n");
        auto other = find(ip, mask);
        if(other == routing.end()) {
//            debug("not found - appending new element to table.\n");
            debug("new network detected - appending to the table: " << inet::get_addr_with_mask(ip, mask));
            routing.emplace_back(ip, mask, route_ip, route_mask, -1);
            other = --routing.end();
        }
        return other;
    }

    void read_table() {
        auto [in, read] = dgram::recv(socket_fd);
        debug("received datagram form " << inet::get_addr(in) << ", content: [ip="
                                        << inet::get_addr(read.network_ip) << ",mask=" << (int)read.mask << ",dist=" << read.dist << "]\n");
        auto route = std::max_element(interfaces.begin(),
                                      interfaces.end(),
                                      best_fit(in));
        int score = best_fit(in).score(*route);
        if(score == -1) {
            std::cerr << "received datagram from not neighbour [sender:" << inet::get_addr(in) << "]" << std::endl;
            //std::exit(EXIT_FAILURE);
            return;
        }
        mark_reachable(*route);
        auto node = find_or_insert(read.network_ip, read.mask, in, route->my_mask);
        route->unreachable_since = turn;
        if(route->network_ip != node->network_ip) {
            node->attempt_update(in, route->my_mask, route->dist, read.dist, turn);
        }
    }

    int min_of_three(uint32_t first, uint32_t second, uint32_t third) const {
        if(first < second) {
            return first < third ? 1 : 3;
        }
        else {
            return second < third ? 2 : 3;
        }
    }

    void update(uint32_t new_route_ip, uint32_t new_route_mask, const dgram &read, const std::vector<interface>::iterator &current_interface,
                std::vector<network_node>::iterator &network) const {
        if(auto natural = std::find_if(interfaces.begin(),
                interfaces.end(),
                is_same_network(interface::get_network(read.network_ip, read.mask), 0)); natural != interfaces.end()) {
            switch (min_of_three(natural->dist, network->dist, current_interface->dist + read.dist)) {
                case 1:
                    network->dist = natural->dist;
                    network->route_addr = natural->network_ip;
                    network->route_mask = natural->my_mask;
                    break;
                case 3:
                    network->dist = current_interface->dist + read.dist;
                    network->route_addr = new_route_ip;
                    network->route_mask = new_route_mask;
                    break;
                default:
                    break;
            }
        }
        else if(new_route_ip == network->route_addr) {
            debug("same route addr.\n");
            if(network_node::is_dist_inf(read.dist)) {
                debug("updating to inf.\n");
                if(!network_node::is_dist_inf(network->dist)) {
                    debug("first message of this kind.\n");
                    network->unreachable_since = turn;
                }
                network->dist = network_node::inf;
            }
            else {
                debug("normal update/possible increase in consts.\n");
                network->dist = read.dist + current_interface->dist;
            }
        }
        else {
            network->update_dist(new_route_ip, new_route_mask, current_interface->dist, read.dist);
        }
    }

    std::string format_table() {
        std::stringstream ss;
        for(const auto& other : routing) {
            if(!network_node::is_dist_inf(other.dist) || turn < other.unreachable_since + 5)
                ss<<other.format() << "\n";
        }
        return ss.str();
    }

    void distribute_table() {
#ifdef DEBUG
        std::cerr << "\n ============== OUTPUT ==============\n"
#else
        std::cout
#endif
        << format_table()
#ifdef DEBUG
        << " ============== !OUTPUT ==============\n";
#else
        << std::endl;
#endif
        for(auto& network : interfaces) {
            debug("\nnetwork: " << inet::get_addr_with_mask(network.network_ip, network.my_mask)
                << ", last heard: " << network.unreachable_since << "\n");
            for(const auto& node : routing) {
                debug("sending update to " << inet::get_addr_with_mask(node.network_ip, node.mask)
                    << " via " << inet::get_addr_with_mask(node.route_addr, node.route_mask) << " on "
                    << inet::get_addr_with_mask(network.broadcast_ip, network.my_mask)
                    << ". current dist: " << node.dist << "\n");
                if(node.send_dist(socket_fd, network.broadcast_ip, network.my_mask)) {
                    debug("sending successful\n");
//                    mark_reachable(network);
                }
                else {
                    debug("sending failed: [" << errno << "] " << std::strerror(errno) << ". Marking as unreachable.\n");
                    mark_unreachable(network);
                }
            }
            if(turn >= network.unreachable_since + 5) {
                debug("no response for 5 turns on "
                    << inet::get_addr_with_mask(network.network_ip, network.my_mask)
                    << ". Marking as unreachable.\n");
                mark_unreachable(network);
            }
        }
    }

    void mark_reachable(interface &network) {
        network.mark_reachable();
        find(network.network_ip, network.my_mask)->dist = network.dist;
    }

    void mark_unreachable(interface &network) {
        if(network.reachable) {
            network.mark_unreachable(turn);
//        find(network.network_ip, network.my_mask)->dist = network_node::inf;
            for (auto &node: routing) {
                if (interface::get_network(node.route_addr, node.route_mask) == network.network_ip &&
                    node.route_mask == network.my_mask) {
                    node.dist = network_node::inf;
                    node.unreachable_since = turn;
                }
            }
        }
    }

    void clear_unreachable() {
//        debug("clearing unreachable\n")
        long before = routing.size();
        routing.erase(std::remove_if(routing.begin(), routing.end(), [&](const network_node& node) {
            if(network_node::is_dist_inf(node.dist)) {
                if(turn >= node.unreachable_since + 5) {
                    if(!node.connected_directly)
                        return true;
                }
            }
            return false;
        }), routing.end());
        debug("clearing done. before " << before << ", after " << routing.size() << "\n");
    }
};

int main(int argc, char* argv[]) {
    try {
        auto init = Init(argc, argv);
        auto interfaces = init.get_interfaces();
#ifdef DEBUG
//        for(auto item : interfaces) {
//            debug("network: " << inet::get_addr(item.network_ip)
//                << ", dist: " << item.dist
//                << ", mask: " << (int)item.my_mask
//                << ", broadcast: " << inet::get_addr(item.broadcast_ip) << "\n");
//        }
#endif
        return router(init.get_interfaces(), init.get_socket_fd()).run();
    }
    catch(std::exception& e) {
        std::cerr << "bad configuration\n";
        return 1;
    };

}
