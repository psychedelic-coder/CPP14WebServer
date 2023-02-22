#include "mymuduo/TcpServer.h"
#include "mymuduo/Logger.h"
#include "mymuduo/TcpConnection.h"

#include <strings.h>

namespace mymuduo
{
    static EventLoop *CheckLoopNotNull(EventLoop *loop)
    {
        if (loop == nullptr)
        {
            LOG_FMT_FATAL("%s:%s:%d mainLoop is null! \n", __FILE__, __FUNCTION__, __LINE__);
        }
        return loop;
    }
    TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr,
                         const std::string &nameArg, Option option)
        : loop_(CheckLoopNotNull(loop)), ipPort_(listenAddr.toIPport()), name_(nameArg), acceptor_(std::make_unique<Acceptor>(loop, listenAddr, option == kReusePort)), threadPool_(std::make_shared<EventLoopThreadPool>(loop, name_)), connectionCallback_(), messageCallback_(), writeCompleteCallback_(), threadInitCallback_(), started_(), nextConnId_(1), connections_()
    {
        // 当有新用户连接时，会执行TCPserver::newConnection回调
        acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1,
                                                      std::placeholders::_2));
    }

    TcpServer::~TcpServer()
    {
        for (auto &item : connections_)
        {
            TcpConnectionPtr conn(item.second); // 这个局部shared_ptr智能指针对象，出右括号，可以自动释放new出来的对象资源
            item.second.reset();
            // 销毁连接
            conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
        }
    }

    // 设置底层subloop的个数
    void TcpServer::setThreadNum(int numThreads)
    {
        threadPool_->setThreadNum(numThreads);
    }

    void TcpServer::start()
    {
        if (started_++ == 0) // 防止一个tcpserver对象被start多次
        {
            threadPool_->start(threadInitCallback_); // 启动底层loop的线程池
            loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
        }
    }

    // 有一个新的客户端的连接，acceptor会执行这个回调操作
    void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
    {
        loop_->assertInLoopThread();
        // 轮询算法，选择一个subloop来管理channel
        EventLoop *ioLoop = threadPool_->getNextLoop();
        char buf[64] = {0};
        snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
        ++nextConnId_;
        std::string connName = name_ + buf;

        LOG_FMT_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n", name_.c_str(), connName.c_str(), peerAddr.toIPport().c_str());
        // 通过sockfd获取其绑定的本机的ip地址和端口信息
        sockaddr_in local;
        ::bzero(&local, sizeof local);
        socklen_t addrlen = sizeof local;
        if (::getsockname(sockfd, (sockaddr *)&local, &addrlen) < 0)
        {
            //LOG_FMT_ERROR("sockets::getLocalAddr");
            LOG_FATAL << "sockets::getLocalAddr, errno: " << errno;
        }
        InetAddress localAddr(local);

        // 根据成功连接的sockfd，创建TcpConnection连接对象
        // TcpConnectionPtr是shared_ptr，使用make_shared分配和使用动态内存给一个对象，这样可以保证因为在runtime的时候
        // 异常发生能够正常回收动态分配的内存。
        TcpConnectionPtr conn(std::make_shared<TcpConnection>(ioLoop, connName, sockfd, localAddr, peerAddr));
        connections_[connName] = conn;
        // 下面的回调都是用户设置给TcpServer=>TcpConnection=>Channel=>Poller=>notify channel调用回调
        conn->setConnectionCallback(connectionCallback_);
        conn->setMessageCallback(messageCallback_);
        conn->setWriteCompleteCallback(writeCompleteCallback_);
        conn->setWriteCompleteCallback(writeCompleteCallback_);
        // 设置了如何关闭连接的回调
        conn->setCloseCallback(std::bind(&TcpServer::removeConenction, this, std::placeholders::_1));
        ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
    }

    void TcpServer::removeConenction(const TcpConnectionPtr &conn)
    {
        loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
    }

    void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
    {
        LOG_FMT_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n",
                 name_.c_str(), conn->name().c_str());
        size_t n = connections_.erase(conn->name());
        EventLoop *ioLoop = conn->getLoop();
        ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }

} // namespace mymuduo
