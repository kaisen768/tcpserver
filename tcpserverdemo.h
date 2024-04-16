#ifndef _TCPSERVERDEMO_H_
#define _TCPSERVERDEMO_H_

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <string>
#include <thread>
#include <list>
#include <functional>

#include "cclqueue/blocking_queue.h"

#define TCPSERVERDEMO_IP_ADDR     "127.0.0.1"
#define TCPSERVERDEMO_PORT        11200
#define TCPSERVERDEMO_CONN_MAX    10
#define TCPSERVERDEMO_SEND_MAX    1024

typedef struct TCPServerDemoClientAttrs {
    struct sockaddr_in sa;
    int fd;

    TCPServerDemoClientAttrs() {
        sa = {0}; fd = -1;
    }
} TCPServerDemoClientAttrs;

typedef std::function<void (int)> TCPServerDemoClientDisconnectedHandleCb;

class TCPServerDemo
{
public:
    TCPServerDemo(const std::string ip = TCPSERVERDEMO_IP_ADDR, const uint16_t port = TCPSERVERDEMO_PORT);
    ~TCPServerDemo();

    int Start();
    void Stop();

    size_t GetConnectNumber();

private:
    void LoopServerHandler();
    void LoopSendHandler();

    int SendtoAllConnect(const uint8_t *data, uint16_t datal);
    int Sendto(const uint8_t *data, uint16_t datal, const TCPServerDemoClientAttrs &client);

private:
    std::thread *loop_thread_;
    std::thread *send_thread_;
    bool online_;
    bool stop_flag_;
    struct sockaddr_in server_addr_;
    int master_fd_;
    std::string server_ip_;
    uint16_t server_port_;
    cclqueue::BlockingQueue queue_;
    std::list<TCPServerDemoClientAttrs> peers_;
    TCPServerDemoClientDisconnectedHandleCb client_disconnect_handle_cb_;
};

#endif // _TCPSERVERDEMO_H_
