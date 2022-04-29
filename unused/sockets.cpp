//
// Created by foxinio on 4/22/22.
//

#include "sockets.h"

#include <unistd.h>

io::io() : fd_in(dup(STDIN_FILENO)), fd_out(dup(STDOUT_FILENO)) {
    current_fds = {fd_in, fd_out};
}

io::io(int fd) : fd_in(fd), fd_out(fd) {}

std::ostream& io::out() {
    if(current_fds.second != fd_out) {
        std::cout.flush();
        dup2(fd_out, STDOUT_FILENO);
        current_fds.second = fd_out;
    }
    return std::cout;
}

std::istream& io::in() {
    if(current_fds.first != fd_in) {
        dup2(fd_in, STDIN_FILENO);
        current_fds.first = fd_in;
    }
    return std::cin;
}
