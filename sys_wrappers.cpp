//
// Created by foxinio on 4/23/22.
//

#include "sys_wrappers.h"

#include <sys/socket.h>
#include <unistd.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <cerrno>


int Bind(int fd, __CONST_SOCKADDR_ARG addr, socklen_t len) {
    int res = bind(fd, addr, len);
    if(res < 0) {
        perror("bind error");
        std::exit(EXIT_FAILURE);
    }
    return res;
}
int Socket(int domain, int type, int protocol) {
    int res = socket(domain, type, protocol);
    if(res < 0) {
        perror("poll error");
        std::exit(EXIT_FAILURE);
    }
    return res;
}


int Poll(int sockfd, int timeout) {
    pollfd fds = {
        .fd=sockfd,
        .events=POLLIN,
        .revents=0
    };

    int res = poll(&fds, 1, timeout);
    if(res < 0) {
        perror("poll error");
        std::exit(EXIT_FAILURE);
    }
    return res;
}

long Recvfrom(int fd, void *buf, size_t n, int flags, sockaddr *addr, socklen_t *addr_len) {
    long res = recvfrom(fd, buf, n, flags, addr, addr_len);
    if(res < 0 && !(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
        perror("recvfrom error");
        std::exit(EXIT_FAILURE);
    }
    return res;
}

long Sendto(int fd, const void *buf, size_t n, int flags, const sockaddr *addr, socklen_t addr_len) {
    return write(fd, buf, n);
    long res = sendto(fd, buf, n, flags, addr, addr_len);
    if(res < 0 && errno == EINTR) {
        res = sendto(fd, buf, n, flags, addr, addr_len);
    }
    return res;
}

int Setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen) {
    int res = setsockopt(fd, level, optname, optval, optlen);
    if(res < 0) {
        perror("setsockopt error");
        std::exit(EXIT_FAILURE);
    }
    return res;
}
