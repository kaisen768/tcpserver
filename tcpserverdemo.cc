#include "tcpserverdemo.h"

#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

TCPServerDemo::TCPServerDemo(const std::string ip, const uint16_t port)
    : loop_thread_(nullptr)
    , send_thread_(nullptr)
    , online_(false)
    , stop_flag_(false)
    , server_addr_{0}
    , master_fd_(-1)
    , server_ip_(ip)
    , server_port_(port)
    , queue_(TCPSERVERDEMO_SEND_MAX)
{
}

TCPServerDemo::~TCPServerDemo()
{
}

int TCPServerDemo::Start()
{
    if ((online_ == true) || (master_fd_ != -1))
        return -1;

    stop_flag_ = false;
    loop_thread_ = new std::thread(&TCPServerDemo::LoopServerHandler, this);
    // send_thread_ = new std::thread(&TCPServerDemo::LoopSendHandler, this);

    return 0;
}

void TCPServerDemo::Stop()
{
    stop_flag_ = true;

    loop_thread_->join();
    delete loop_thread_;
    loop_thread_ = nullptr;

    // send_thread_->join();
    // delete send_thread_;
}

size_t TCPServerDemo::GetConnectNumber()
{
    if (online_ != true)
        return 0;

    return peers_.size();
}

void TCPServerDemo::LoopServerHandler()
{
    int err;
    fd_set rdfdset;
    int max_fd;
    int on;

    master_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (master_fd_ < 0) {
        fprintf(stderr, "socket create failed !\n");
        return;
    }

    on = 1;
    err = setsockopt(master_fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (err) {
        fprintf(stderr, "socket setsockopt failed, err:%d !\n", err);
        goto error0;
    }

    server_addr_.sin_family = AF_INET;
    server_addr_.sin_addr.s_addr = inet_addr(server_ip_.c_str());
    server_addr_.sin_port = htons(server_port_);

    err = bind(master_fd_, (const sockaddr*)&server_addr_, sizeof(server_addr_));
    if (err) {
        fprintf(stderr, "socket bind address failed, err:%d !\n", err);
        goto error0;
    }

    err = listen(master_fd_, TCPSERVERDEMO_CONN_MAX);
    if (err) {
        fprintf(stderr, "socket listen failed, err:%d !\n", err);
        goto error0;
    }

    fprintf(stderr, "tcp server start success.\n");

    max_fd = master_fd_;
    online_ = true;

    while (!stop_flag_) {
        struct timeval tv = {0, 500000};
        FD_ZERO(&rdfdset);

        FD_SET(master_fd_, &rdfdset);
        for (auto itr = peers_.begin();
             itr != peers_.end(); itr++) {
            FD_SET(itr->fd, &rdfdset);
        }

        err = select(max_fd + 1, &rdfdset, nullptr, nullptr, &tv);
        if (err == 0)
            continue;
        else if (err < 0)
            continue;

        if (FD_ISSET(master_fd_, &rdfdset)) {
            TCPServerDemoClientAttrs peer;
            socklen_t len = sizeof(peer.sa);
            peer.fd = accept(master_fd_, (struct sockaddr*)&peer.sa, &len);
            if (peer.fd <= 0) {
                fprintf(stderr, "socket accept failed, err:%d !\n", peer.fd);
            } else {
                fprintf(stderr, "Client [%s:%d] connected!\n", inet_ntoa(peer.sa.sin_addr), 
                                                             ntohs(peer.sa.sin_port));
                peers_.push_back(peer);
                max_fd = (peer.fd > max_fd) ? peer.fd : max_fd;
            }
        } else {
            for (auto itr = peers_.begin(); itr != peers_.end();) {
                struct sockaddr_in *sa = &itr->sa;
                int active_fd = itr->fd;
                char *recvbuf = nullptr;
                bool do_erase = false;

                if (FD_ISSET(active_fd, &rdfdset)) {
                    recvbuf = static_cast<char*>(calloc(sizeof(char), 4096));
                    err = recv(active_fd, recvbuf, 4096, 0);
                    if (err == 0) {
                        fprintf(stderr, "Client [%s:%d] disconnected!\n", inet_ntoa(sa->sin_addr),
                                                                        ntohs(sa->sin_port));
                        do_erase = true;
                        close(active_fd);

                        if (client_disconnect_handle_cb_ != nullptr) {
                            client_disconnect_handle_cb_(active_fd);
                        }
                    } else if (err > 0) {
                        std::string msgdata;
                        msgdata.assign(recvbuf, err);

                        Sendto((uint8_t *)recvbuf, err, *itr);
                    }

                    free(recvbuf);
                }

                if (do_erase)
                    itr = peers_.erase(itr);
                else
                    itr++;
            }
        }
    }

    for (auto itr = peers_.begin(); itr != peers_.end();) {
        if (itr->fd > 0)
            close(itr->fd);
        itr = peers_.erase(itr);
    }

    online_ = false;

error0:
    close(master_fd_);
    return;
}

int TCPServerDemo::SendtoAllConnect(const uint8_t *data, uint16_t datal)
{
    int err;
    int num = 0;

    if ((data == nullptr) || (datal == 0))
        return -1;

    for (auto itr = peers_.begin(); itr != peers_.end(); itr++) {
        TCPServerDemoClientAttrs cli = *itr;
        
        err = Sendto(data, datal, cli);
        if (err > 0)
            num++;
    }

    return num;
}

int TCPServerDemo::Sendto(const uint8_t *data, uint16_t datal, const TCPServerDemoClientAttrs &peer)
{
    int r;

    if ((data == nullptr) || (datal == 0))
        return -1;

    r = send(peer.fd, data, datal, MSG_NOSIGNAL);
    if (r <= 0)
        return -1;

    return r;
}
