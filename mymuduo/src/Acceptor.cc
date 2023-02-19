#include "mymuduo/Acceptor.h"
#include "mymuduo/Logger.h"
#include "mymuduo/InetAddress.h"
#include "mymuduo/EventLoop.h"
#include "mymuduo/Acceptor.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

namespace mymuduo
{
    static int createNoneBlockingOrDie()
    {
        int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (sockfd < 0)
        {
            LOG_FMT_FATAL("%s:%s:%d listen socket create err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
        }

        return sockfd;
    }

    Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
        : loop_(loop), acceptSocket_(createNoneBlockingOrDie()), acceptChannel_(loop, acceptSocket_.fd()), listenning_(false)
    {
        acceptSocket_.setKeepAlive(true);
        acceptSocket_.setReuseAddr(true);
        acceptSocket_.bindAddress(listenAddr); // bind
        // TCPServer::start() Acceptor listen    有新用户的连接，要执行一个回调
        acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
    }

    Acceptor::~Acceptor()
    {
        acceptChannel_.disableAll();
        acceptChannel_.remove();
    }

    void Acceptor::listen()
    {
        loop_->assertInLoopThread();
        listenning_ = true;
        acceptSocket_.listen();
        acceptChannel_.enableReading();
    }

    // listenfd有事件发生了，就是有新用户连接了
    void Acceptor::handleRead()
    {
        InetAddress peerAddr;
        int connfd = acceptSocket_.accept(&peerAddr);
        if (connfd >= 0)
        {
            if (newConnectionCallback_)
            {
                newConnectionCallback_(std::move(acceptSocket_).accept(&peerAddr), peerAddr); // 轮询找到subloop，唤醒，分发新客户端当前的Channel
            }
            else
            {
                ::close(connfd);
            }
        }
        else
        {
            LOG_FMT_ERROR("%s:%s:%d accept create err: %d \n", __FILE__, __FUNCTION__, __LINE__, errno);
            if (errno == EMFILE)
            {
                LOG_FMT_ERROR("%s:%s:%d sockfd reach the limit \n", __FILE__, __FUNCTION__, __LINE__);
            }
        }
    }
} // namespace mymuduo
