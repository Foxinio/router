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

    const seconds timeout = 5s;

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

            debug("entering Poll with wait time: " << std::max((int)duration.count(), 0) << "ms\n");
            if(Poll(socket_fd, std::max((int)duration.count(), 0)) == 0) {
                debug("time's up, distributing table\n");
                distribute_table();
                clear_unreachable();
                last_sent = time_point_cast<milliseconds>(high_resolution_clock::now());
                turn++;
            }
            else {
                debug("received datagram\n");
                read_table();
            }
        }
    }

    std::vector<network_node>::iterator find(uint32_t ip, uint8_t mask) {
        return std::find_if(routing.begin(), routing.end(), is_same_network(ip, mask));
    }

    std::vector<network_node>::iterator find_or_insert(uint32_t ip, uint8_t mask, uint32_t route_ip) {
        debug("searching routing_table for " << inet::get_addr_with_mask(ip, mask) << "\n");
        auto other = find(ip, mask);
        if(other == routing.end()) {
            debug("not found - appending new element to table.\n");
            routing.emplace_back(ip, route_ip, mask, -1);
            other = --routing.end();
        }
        return other;
    }

    void read_table() {
        auto [in, read] = dgram::recv(socket_fd);
        debug("received datagram form " << inet::get_addr(in) << ", content: [ip="
            << inet::get_addr(read.ip) << ",mask=" << (int)read.mask << ",dist=" << read.dist << "]\n");
        auto route = std::find_if(interfaces.begin(),
                            interfaces.end(),
                            is_same_network(in, 0));
        if(route == interfaces.end()) {
            std::cerr << "received datagram from not neighbour [sender:" << inet::get_addr(in) << "]" << std::endl;
            //std::exit(EXIT_FAILURE);
            return;
        }
        mark_reachable(*route);
        auto other = find_or_insert(read.ip, read.mask, in);
        route->unreachable_since = turn;
        other->attempt_update(in, route->dist, read.dist, turn);
    }

    std::string format_table() {
        std::stringstream ss;
        for(const auto& other : routing) {
            ss<<other.format() << "\n";
        }
        return ss.str();
    }

    void distribute_table() {
        std::cout << format_table();
        for(auto& network : interfaces) {
            debug("network: " << inet::get_addr_with_mask(network.network_ip, network.my_mask)
                << ", last heard: " << network.unreachable_since << "\n");
            for(const auto& node : routing) {
                debug("sending update to " << inet::get_addr_with_mask(node.network_ip, node.mask)
                    << " via " << inet::get_addr(node.route_addr) << " on "
                    << inet::get_addr_with_mask(network.broadcast_ip, network.my_mask)
                    << ". current dist: " << node.dist << "\n");
                if(node.send_dist(socket_fd, network.broadcast_ip)) {
                    debug("sending successful\n");
                    mark_unreachable(network);
                }
                else {
                    debug("sending failed: [" << errno << "] " << std::strerror(errno) << "\n");
                    mark_reachable(network);
                }
            }
            if(turn > network.unreachable_since + 5) {
                debug("no responce for 5 turns on "
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
        network.mark_unreachable(turn);
        find(network.network_ip, network.my_mask)->dist = network_node::inf;
    }

    void clear_unreachable() {
        debug("clearing unreachable\n");
        int before = routing.size();
        routing.erase(std::remove_if(routing.begin(), routing.end(), [&](const network_node& node) {
            return network_node::is_dist_inf(node.dist)
                   && turn < node.unreachable_since + 5
                   && !node.connected_directly;
        }), routing.end());
        int after = routing.size();
        debug("clearing done. before " << before << ", after " << after << "\n");
    }
};

int main(int argc, char* argv[]) {
    try {
        auto init = Init(argc, argv);
        auto interfaces = init.get_interfaces();
        for(auto item : interfaces) {
            debug("network: " << inet::get_addr(item.network_ip)
                << ", dist: " << item.dist
                << ", mask: " << (int)item.my_mask
                << ", broadcast: " << inet::get_addr(item.broadcast_ip) << "\n");
        }
        return router(init.get_interfaces(), init.get_socket_fd()).run();
    }
    catch(std::exception& e) {
        std::cerr << "bad configuration\n";
        return 1;
    };

}
