#include "mymuduo/TcpConnection.h"
#include "mymuduo/Logger.h"
#include "mymuduo/Socket.h"
#include "mymuduo/Channel.h"
#include "mymuduo/EventLoop.h"

#include <functional>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>
#include <string>

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
    TcpConnection::TcpConnection(EventLoop *loop, const std::string &name, int sockfd, const InetAddress &localAddr,
                                 const InetAddress &peerAddr)
        : loop_(CheckLoopNotNull(loop)) // 这里绝对不是baseloop,因为TcpConnection都是在subloop里面管理的
          ,
          name_(name), state_(kConnecting), reading_(true), socket_(std::make_unique<Socket>(sockfd)), channel_(std::make_unique<Channel>(loop, sockfd)), localAddr_(localAddr), peerAddr_(peerAddr), highWaterMark_(64 * 1024 * 1024)
    {
        // 给channel设置相应的回调函数，poller给Channel通知感兴趣的事情发生了，channel会回调相应的操作函数
        channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
        channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
        channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
        channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

        LOG_FMT_INFO("TcpConnection::ctor[%s] at fd=%d \n", name_.c_str(), sockfd);
        socket_->setKeepAlive(true);
    }

    TcpConnection::~TcpConnection()
    {
        LOG_FMT_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n", name_.c_str(), channel_->fd(), (int)state_);
    }

    void TcpConnection::send(const std::string &buf)
    {
        if (state_ == kConnected)
        {
            if (loop_->isInLoopThread())
            {
                sendInLoop(buf.c_str(), buf.size());
            }
            else
            {
                // sendInLoop有多重重载，需要使用函数指针确定
                // 遇到重载函数的绑定，可以使用函数指针来指定确切的函数
                void (TcpConnection::*fp)(const void* data, size_t len) = &TcpConnection::sendInLoop;
                loop_->runInLoop(std::bind(fp, this, buf.c_str(), buf.size()));
            }
        }
    }

    void TcpConnection::send(Buffer *buf)
    {
        if (state_ == kConnected)
        {
            if (loop_->isInLoopThread())
            {
                sendInLoop(buf->peek(), buf->readableBytes());
                buf->retrieveAll();
            }
            else
            {
                // sendInLoop有多重重载，需要使用函数指针确定
                void (TcpConnection::*fp)(const std::string &message) = &TcpConnection::sendInLoop;
                loop_->runInLoop(std::bind(fp, this, buf->retrieveAllAsString()));
            }
        }
    }

    void TcpConnection::shutdown()
    {
        if (state_ == kConnected)
        {
            setState(kDisconnecting);
            loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
        }
    }

    void TcpConnection::setTcpNoDelay(bool on)
    {
        socket_->setTcpNoDelay(on);
    }

    void TcpConnection::shutdownInLoop()
    {
        if (!channel_->isWriting()) // 说明当前output buffer中的数据已经全部发送完成
        {
            socket_->shutdownWrite();
        }
    }

    // 连接建立
    void TcpConnection::connectEstablished()
    {
        setState(kConnected);
        channel_->tie(shared_from_this());
        channel_->enableReading(); // 向poller注册channel的epollin事件

        // 新连接建立，执行回调
        connectionCallback_(shared_from_this());
    }

    // 连接销毁
    void TcpConnection::connectDestroyed()
    {
        if (state_ == kConnected)
        {
            setState(kDisconnected);
            channel_->disableAll();

            connectionCallback_(shared_from_this());
        }

        channel_->remove(); // 把channel从poller中删除掉
    }

    void TcpConnection::handleRead(Timestamp receiveTime)
    {
        int savedErrno = 0;
        ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
        if (n > 0)
        {
            // 已建立连接的用户，有可读事件发生了，调用用户传入的回调操作onMessage
            messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
        }
        else if (n == 0)
        {
            // 客户端断开
            handleClose();
        }
        else
        {
            errno = savedErrno;
            LOG_FMT_ERROR("TcpConnection::handleRead");
            handleError();
        }
    }

    void TcpConnection::handleWrite()
    {
        if (channel_->isWriting())
        {
            int savedErrno = 0;
            ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
            if (n > 0)
            {
                outputBuffer_.retrieve(n);
                // 一旦发送完outputBuffer_的数据，就停止观察writable事件避免busyloop.
                if (outputBuffer_.readableBytes() == 0)
                {
                    channel_->disableWriting();
                    if (writeCompleteCallback_)
                    {
                        // 唤醒loop_对应的thread线程执行回调
                        loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                    }
                    // 发送完outputBuffer_的数据且这时连接正在关闭，就继续执行关闭TcpConnection过程。
                    if (state_ == kDisconnecting)
                    {
                        shutdownInLoop();
                    }
                }
            }
            else
            {
                LOG_FMT_ERROR("TcpConnection::handleWrite");
            }
        }
        else
        {
            LOG_FMT_ERROR("TcpConnection fd=%d is down, no more writing \n", channel_->fd());
        }
    }

    void TcpConnection::handleClose()
    {
        LOG_FMT_INFO("fd=%d state=%d \n", channel_->fd(), (int)state_);
        setState(kDisconnected);
        channel_->disableAll();

        TcpConnectionPtr connPtr(shared_from_this());
        connectionCallback_(connPtr); // 连接关闭的回调
        closeCallback_(connPtr);      // 关闭连接的回调
    }

    void TcpConnection::handleError()
    {
        int optval;
        socklen_t optlen = sizeof optval;
        int err = 0;
        if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
        {
            err = errno;
        }
        else
        {
            err = optval;
        }
        LOG_FMT_ERROR("TcpConnection::handleError name: %s - SO_ERROR:%d \n", name_.c_str(), err);
    }

    // 发送数据 应用写的快，而内核发送数据慢，需要把待发送数据写入缓冲区，而且设置了水位回调
    void TcpConnection::sendInLoop(const void *data, size_t len)
    {
        size_t nwrote = 0;
        size_t remaining = len;
        bool faultError = false;

        // 之前调用过connection的shutdown，不能再进行发送了
        if (state_ == kDisconnected)
        {
            LOG_FMT_ERROR("disconnected, give up writing!");
            return;
        }

        // 表示channel_第一次开始写数据，而且缓冲区没有待发送数据
        if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
        {
            nwrote = ::write(channel_->fd(), data, len);
            if (nwrote >= 0)
            {
                remaining = len - nwrote;
                if (remaining == 0 && writeCompleteCallback_)
                {
                    // 既然在这里数据全部发送完成，就不用再给channel设置epollout事件了
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
            }
            else // nwrote 0
            {
                nwrote = 0;
                if (errno != EWOULDBLOCK)
                {
                    LOG_FMT_ERROR("TcpConnection::sendInLoop");
                    if (errno == EPIPE || errno == ECONNRESET) // SIGPIPE SIGRESET
                    {
                        faultError = true;
                    }
                }
            }
        }

        // 说明当前这一次write，并没有把数据全部发送出去，剩余的数据需要保存到缓冲区当中
        // 然后给channel注册epollout事件，poller发现tcp的发送缓冲区有空间，会通知相应的sock-channel，调用handleWrite回调方法
        // 也就是调用TcpConnection::handleWrite方法，把发送缓冲区中的数据全部发送完成
        if (!faultError && remaining > 0)
        {
            size_t oldLen = outputBuffer_.readableBytes();
            if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMark_)
            {
                loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
            }
            outputBuffer_.append((char *)data + nwrote, remaining);
            if (!channel_->isWriting())
            {
                channel_->enableWriting(); // 注册channel的写事件
            }
        }
    }

    void TcpConnection::sendInLoop(const std::string &message)
    {
        sendInLoop(message.data(), message.size());
    }

} // namespace mymuduo
