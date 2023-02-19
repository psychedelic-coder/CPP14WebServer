#pragma once

/**
 * @brief 用户使用mymuduo库的TCPserver类编写服务器程序，为了不暴露给用户过多头文件
 * 在这里包含了相应的头文件
 */
#include "mymuduo/EventLoop.h"
#include "mymuduo/Acceptor.h"
#include "mymuduo/InetAddress.h"
#include "mymuduo/EventLoopThreadPool.h"
#include "mymuduo/noncopyable.h"
#include "mymuduo/Callbacks.h"

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

namespace mymuduo
{
    // 对外的服务器编程使用的类
    class TcpServer : noncopyable
    {
    public:
        using ThreadInitCallback = std::function<void(EventLoop *)>;

        enum Option
        {
            kNoReusePort,
            kReusePort,
        };

        TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg, Option option = kNoReusePort);
        ~TcpServer();

        const std::string &ipPort() const { return ipPort_; }
        const std::string &name() const { return name_; }
        EventLoop *getLoop() const { return loop_; }
        // 设置底层subloop的个数
        void setThreadNum(int numThreads);

        void setThreadInitCallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; }
        void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
        void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
        void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

        // 开启服务器监听
        void start();

    private:
        void newConnection(int sockfd, const InetAddress &peerAddr);
        void removeConenction(const TcpConnectionPtr &conn);
        void removeConnectionInLoop(const TcpConnectionPtr &conn);

        using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

        EventLoop *loop_; // the acceptor loop，也就是baseloop 用户定义的loop

        const std::string ipPort_;
        const std::string name_;

        const std::unique_ptr<Acceptor> acceptor_;
        std::shared_ptr<EventLoopThreadPool> threadPool_; // one loop per thread
        ConnectionCallback connectionCallback_;           // 有新连接
        MessageCallback messageCallback_;                 // 有读写消息时的回调
        WriteCompleteCallback writeCompleteCallback_;     // 消息发送完成以后的回调
        ThreadInitCallback threadInitCallback_;           // loop线程初始化的回调
        std::atomic_int started_;

        int nextConnId_;
        ConnectionMap connections_; // 所有的连接
    };

} // namespace mymuduo