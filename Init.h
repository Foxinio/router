// Szymon Jedras, i322920

#include "network_node.h"

#include <iostream>
#include <vector>

class Init {
    char** argv;
    int argc;

    interface_table interfaces;
    int socket_fd;
public:

    Init(int argc, char* argv_[]);
    ~Init();

    interface_table get_interfaces();
    int get_socket_fd() const;

private:
    void handle_init();
    interface_table read_input();

};

