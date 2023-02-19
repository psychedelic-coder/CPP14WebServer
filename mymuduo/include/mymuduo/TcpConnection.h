#pragma once

#include "mymuduo/noncopyable.h"
#include "mymuduo/InetAddress.h"
#include "mymuduo/Callbacks.h"
#include "mymuduo/Buffer.h"
#include "mymuduo/Timestamp.h"

#include <memory.h>
#include <string>
#include <atomic>

namespace mymuduo
{
    class Channel;
    class EventLoop;
    class Socket;
    // TcpConnection表示的是“一次TCP连接”，它是不可再生的，一旦连接断开，这个TcpConnection对象就没啥用了。
    // 而且TcpConnection没有发起连接的功能，其构造函数参数是已经建立好的sockfd，因此其初始状态是kConnecting。
    class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
    {
    public:
        TcpConnection(EventLoop *loop, const std::string &name, int sockfd, const InetAddress &localAddr,
                      const InetAddress &peerAddr);
        ~TcpConnection();

        EventLoop *getLoop() const { return loop_; }
        const std::string &name() const { return name_; }
        const InetAddress &localAddress() const { return localAddr_; }
        const InetAddress &peerAddress() const { return peerAddr_; }

        bool connected() const { return state_ == kConnected; }

        // void send(const void* message, size_t len);
        // Thread safe
        void send(const std::string &buf);
        void send(Buffer *buf);

        // Thread safe
        void shutdown();
        void setTcpNoDelay(bool on);

        void setConnectionCallback(const ConnectionCallback &cb)
        {
            connectionCallback_ = cb;
        }

        void setMessageCallback(const MessageCallback &cb)
        {
            messageCallback_ = cb;
        }

        void setWriteCompleteCallback(const WriteCompleteCallback &cb)
        {
            writeCompleteCallback_ = cb;
        }

        void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark)
        {
            highWaterMarkCallback_ = cb;
            highWaterMark_ = highWaterMark;
        }

        /// Internal use only.
        void setCloseCallback(const CloseCallback &cb)
        {
            closeCallback_ = cb;
        }

        // called when TcpServer accepts a new connection
        void connectEstablished();  // should be called only once
        // called when TcpServer has removed me from its map
        void connectDestroyed();    // should be called only once

    private:
        enum StateE
        {
            kDisconnected,
            kConnecting,
            kConnected,
            kDisconnecting
        };

        void setState(StateE state) { state_ = state; }

        void handleRead(Timestamp receiveTime);
        void handleWrite();
        void handleClose();
        void handleError();

        void sendInLoop(const void *data, size_t len);
        void sendInLoop(const std::string &message);
        void shutdownInLoop();

        EventLoop *loop_; // 这里绝对不是baseloop,因为TcpConnection都是在subloop里面管理的
        const std::string name_;
        std::atomic_int state_;
        bool reading_;

        // 这里和Acceptor类似
        std::unique_ptr<Socket> socket_;
        std::unique_ptr<Channel> channel_;

        const InetAddress localAddr_; // 主机
        const InetAddress peerAddr_;  // 客户端

        ConnectionCallback connectionCallback_;       // 保存所有的连接
        MessageCallback messageCallback_;             // 有读写消息时的回调
        WriteCompleteCallback writeCompleteCallback_; // 消息发送完成以后的回调
        CloseCallback closeCallback_;
        HighWaterMarkCallback highWaterMarkCallback_;

        size_t highWaterMark_;
        Buffer inputBuffer_;  // 接收数据的缓冲区
        Buffer outputBuffer_; // 发送数据的缓冲区
    };

} // namespace mymuduo
