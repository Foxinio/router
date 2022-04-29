#include "utils.h"

#include "Init.h"
#include "dgram.h"
#include "sys_wrappers.h"


#include <iostream>
#include <utility>
#include <chrono>
#include <sstream>
#include <algorithm>

using namespace std::chrono;
using namespace std::chrono_literals;

class router {
    interface_table interfaces;
    routing_table routing;
    int socket_fd;

    uint32_t turn;

public:
    router(interface_table&& interfaces, int socket_fd)
        : interfaces(std::move(interfaces))
        , socket_fd(socket_fd)
        , turn(0) {
        for(auto item : this->interfaces) {
            routing.emplace_back(item.network_ip, item.my_mask, item.dist);
        }
    }

    int run() {
        auto last_sent = time_point_cast<milliseconds>(high_resolution_clock::now());
        while(true) {
            auto duration = last_sent + 30s - high_resolution_clock::now();

            if(Poll(socket_fd, std::max((int)duration.count(), 0)) == 0) {
                distribute_table();
                clear_unreachable();
                last_sent = time_point_cast<milliseconds>(high_resolution_clock::now());
                turn++;
            }
            else {
                read_table();
            }
        }
    }

    std::vector<network_node>::iterator find_or_insert(uint32_t ip, uint8_t mask, uint32_t route_ip) {
        auto other = std::find_if(routing.begin(), routing.end(), is_same_network(ip, mask));
        if(other == routing.end()) {
            routing.emplace_back(ip, route_ip, mask, -1);
            other = --routing.end();
        }
        return other;
    }

    std::vector<interface>::iterator find_neighbour(uint32_t network_ip) {
        return std::find_if(interfaces.begin(),
                            interfaces.end(),
                            is_member_equal(network_ip, &interface::network_ip));
    }

    void read_table() {
        auto [in, read] = dgram::recv(socket_fd);
        auto route = std::find_if(interfaces.begin(),
                            interfaces.end(),
                            is_same_network(in, 0));
        if(route == interfaces.end()) {
            std::cerr << "received datagram from not neighbour [sender:" << in << "]" << std::endl;
            //std::exit(EXIT_FAILURE);
            return;
        }
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
            for(const auto& node : routing) {
                if(!node.send_dist(socket_fd, network.broadcast_ip)) {
                    network.mark_unreachable(turn);
                }
                else {
                    network.mark_reachable();
                }
            }
            if(turn > network.unreachable_since + 5) {
                network.mark_unreachable(turn);
            }
        }
    }

    void clear_unreachable() {
        routing.erase(std::remove_if(routing.begin(), routing.end(), [&](const network_node& node) {
            return network_node::is_dist_inf(node.dist)
                   && turn < node.unreachable_since + 5
                   && !node.connected_directly;
        }), routing.end());
    }
};

int main(int argc, char* argv[]) {
    auto init = Init(argc, argv);
	return router(init.get_interfaces(), init.get_socket_fd()).run();
}
