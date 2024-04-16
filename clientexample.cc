#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <thread>
#include <chrono>

#define TCPSERVERDEMO_IP_ADDR     "127.0.0.1"
#define TCPSERVERDEMO_PORT        11200

class TCPClientExample
{
public:
    TCPClientExample();
    ~TCPClientExample();

    int Start();
    void Stop();

    bool GetConnectedStatus();

    int Sendto(const uint8_t *data, uint16_t datal);

private:
    void LoopThreadHandler();

private:
    std::thread *loop_thread_;
    struct sockaddr_in saddr_;
    int connfd_;
    bool online_;
    bool stop_flag_;
    char *recvbuf_;
};

TCPClientExample::TCPClientExample()
    : loop_thread_(nullptr)
    , saddr_{0}
    , connfd_(-1)
    , online_(false)
    , stop_flag_(false)
{
    recvbuf_ = (char*)malloc(65535);
}

TCPClientExample::~TCPClientExample()
{
    if (recvbuf_ != nullptr)
        free(recvbuf_);
}

int TCPClientExample::Start()
{
    if (online_ != false)
        return -1;

    stop_flag_ = false;
    loop_thread_ = new std::thread(&TCPClientExample::LoopThreadHandler, this);

    return 0;   
}

void TCPClientExample::Stop()
{
    stop_flag_ = true;
    while (stop_flag_ == true) {
        usleep(10000);
    }
    
    loop_thread_->join();
    delete loop_thread_;
    loop_thread_ = nullptr;

    connfd_ = -1;
    memset(&saddr_, 0, sizeof(saddr_));
}

bool TCPClientExample::GetConnectedStatus()
{
    return online_;
}

void TCPClientExample::LoopThreadHandler()
{
    int err;
    int sockfd;
    fd_set rdfdset;
    int max_fd;

    saddr_.sin_family = AF_INET;
    saddr_.sin_addr.s_addr = inet_addr(TCPSERVERDEMO_IP_ADDR);
    saddr_.sin_port = htons(TCPSERVERDEMO_PORT);

    while (1) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            fprintf(stderr, "socket create failed !, err: %d\n", sockfd);
            goto error0;
        }

        err = connect(sockfd, (struct sockaddr*)&saddr_, sizeof(saddr_));
        if (err) {
            fprintf(stderr, "socket connect failed !, err: %d\n", err);
            goto error1;
        }

        online_ = true;
        connfd_ = sockfd;
        max_fd = sockfd;

        while (!stop_flag_) {
            struct timeval tv = {0, 500000};

            FD_ZERO(&rdfdset);
            FD_SET(sockfd, &rdfdset);

            err = select(max_fd + 1, &rdfdset, nullptr, nullptr, &tv);
            if (err == 0)
                continue;
            else if (err < 0)
                continue;

            if (!FD_ISSET(sockfd, &rdfdset))
                continue;

            if (!recvbuf_) {
                fprintf(stderr, "ipcomm client recvbuffer is null !\n");
                continue;
            }

            memset(recvbuf_, 0, 65536);
            err = recv(sockfd, recvbuf_, 65536, MSG_NOSIGNAL);
            if (err == 0) {
                break;
            } else if (err < 0) {
                continue;
            } else {
                fprintf(stderr, "%s\n", recvbuf_);
            }
        }

        online_ = false;

error1:
        close(sockfd);
        connfd_ = -1;
error0:
        if (stop_flag_ == true)
            break;
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }

    stop_flag_ = false;
}

int TCPClientExample::Sendto(const uint8_t *data, uint16_t datal)
{
    if (online_ == false)
        return -1;

    if ((data == nullptr) || (datal == 0))
        return -1;

    return send(connfd_, data, datal, MSG_NOSIGNAL);
}


int main(int argc, char const *argv[])
{
    std::cout << "TCP Client Example Start" << std::endl;

    TCPClientExample client;

    client.Start();

    while (1) {
        uint8_t message[512] = "Hello server";

        int r;
        r = client.Sendto(message, strlen((char *)message));
        fprintf(stderr, "send return : %d\n", r);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // sleep(1);
    }

    client.Stop();

    return 0;
}
