#include <iostream>
#include <fcntl.h>
#include <unistd.h>

void magic_fun() {
    int fd = open("file.txt", O_RDWR);
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    close(fd);
}

int main() {
    std::cout << "Enter your name: ";
    std::string in;
    std::getline(std::cin, in);
    std::cout << "Hello " << in << std::endl;
//    std::cerr << "[ERR] Opening " << in << std::endl;
    std::getline(std::cin, in);
    std::cout << "Writing to the file: " << in << std::endl;
    std::cerr << "[ERR] Read from file: " << in << std::endl;
}
