#include <iostream>
#include "tcpserverdemo.h"

int main(int argc, char const *argv[])
{
    std::cout << "TCP Server Example Start" << std::endl;

    TCPServerDemo tcpserver;

    tcpserver.Start();

    // Just block till user tells us to quit.
    std::cout << "please enter the charater 'q' to exit." << std::endl;
    while (std::tolower(std::cin.get()) != 'q')
        ;

    tcpserver.Stop();

    return 0;
}
