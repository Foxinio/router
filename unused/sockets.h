//
// Created by foxinio on 4/22/22.
//
#pragma once

#include <iostream>


// this class is not thread safe
class io {
    int fd_in;
    int fd_out;

    static std::pair<int,int> current_fds;

    io();
public:
    static io std;

    io(int fd);

    std::ostream& out();
    std::istream& in();
};


class io& stdio = io::std;
