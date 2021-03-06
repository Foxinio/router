// Szymon Jedras, i322920

#include "utils.h"
#include "Init.h"
#include "dgram.h"
#include "sys_wrappers.h"

#include <iostream>
#include <utility>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <map>
#include <cstring>

using namespace std::chrono;
using namespace std::chrono_literals;

class router {
    interface_table interfaces;
    routing_table routing;
    std::map<uint32_t,uint32_t> last_heard;
    int socket_fd;

    uint32_t turn;

    const seconds timeout = 3s;

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
                deal_with_oposites();
                last_sent = time_point_cast<milliseconds>(high_resolution_clock::now());
                turn++;
            }
            else {
                debug("\treceived datagram\n");
                read_table();
            }
        }
    }

    void deal_with_oposites() {
        for(auto it = last_heard.begin(); it != last_heard.end();) {
            auto [addr, last_turn] = *it;
            if(turn >= last_turn + 5) {
                for (auto &node: routing) {
                    if (node.route_addr == addr) {
                        node.set_unreachable(turn);
                    }
                }
                last_heard.erase(it++);
            }
            else {
                ++it;
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
        last_heard[in] = turn;
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
        if(in != route->my_ip) {
            node->attempt_update(in, route->my_mask, route->dist, read.dist, turn, route->network_ip == node->network_ip);
        }
    }

    std::string format_table() const {
        std::stringstream ss;
        for(const auto& other : routing) {
            if(!network_node::is_dist_inf(other.dist) || turn < other.unreachable_since + 5)
                ss<<other.format() << "\n";
        }
        return ss.str();
    }

    void distribute_table() {
        auto format = format_table();
#ifdef DEBUG
        std::cerr << "\n ============== OUTPUT ==============\n"
        << "[TURN:" << turn << "]\n"
        << format
        << " ============== !OUTPUT ==============\n";
#endif
        std::cout
        << "[TURN:" << turn << "]\n"
        << format
        << std::endl;
        for(auto& network : interfaces) {
            debug("\nnetwork: " << inet::get_addr_with_mask(network.network_ip, network.my_mask)
                << ", last heard: " << network.unreachable_since << "\n");
            for(const auto& node : routing) {
                debug("sending update to " << inet::get_addr_with_mask(node.network_ip, node.mask)
                    << " via " << inet::get_addr_with_mask(node.route_addr, node.route_mask) << " on "
                    << inet::get_addr_with_mask(network.broadcast_ip, network.my_mask)
                    << ". current dist: " << node.dist << "\n");
                if(node.send_dist(socket_fd, network.broadcast_ip, network.my_mask, turn)) {
                    debug("sending successful\n");
//                    mark_reachable(network);
                }
                else {
//                    debug("sending failed: [" << errno << "] " << std::strerror(errno) << ". Marking as unreachable.\n");
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
        auto it = find(network.network_ip, network.my_mask);
        if(it->dist >= network.dist) {
            routing.erase(it);
            routing.emplace_back(network.network_ip, network.my_mask, network.dist);
        }
    }

    void mark_unreachable(interface &network) {
        if(network.reachable) {
            network.mark_unreachable(turn);
            for (auto &node: routing) {
                if (interface::get_network(node.route_addr, node.route_mask) == network.network_ip &&
                    node.route_mask == network.my_mask) {
                    node.set_unreachable(turn);
                }
            }
        }
    }

    void clear_unreachable() {
//        debug("clearing unreachable\n")
        debug("";long before = routing.size());
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
