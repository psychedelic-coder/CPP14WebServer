#pragma once

#include "mymuduo/noncopyable.h"
#include "mymuduo/Socket.h"
#include "mymuduo/Channel.h"

#include <functional>

namespace mymuduo
{
    class EventLoop;
    class InetAddress;
    class Acceptor : noncopyable
    {
    public:
        // NewConnectionCallback原版本直接传递的是socket fd，这种传递int句柄的做法不够理想
        // 改为直接传递一个Socket右值引用，能够确保资源的安全释放。
        // Socket noncopyable
        using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;
        Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
        ~Acceptor();

        void setNewConnectionCallback(const NewConnectionCallback &cb)
        {
            newConnectionCallback_ = std::move(cb);
        }

        bool listenning() { return listenning_; }
        void listen();

    private:
        void handleRead();

        EventLoop *loop_; // Acceptor用的就是用户定义的那个baseloop,也称作mainloop
        Socket acceptSocket_;
        Channel acceptChannel_;
        NewConnectionCallback newConnectionCallback_;
        bool listenning_;
    };
} // namespace mymuduo
