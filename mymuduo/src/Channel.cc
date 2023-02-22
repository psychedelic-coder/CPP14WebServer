#include "mymuduo/Channel.h"
#include "mymuduo/EventLoop.h"
#include "mymuduo/Logger.h"

#include <sys/epoll.h>
#include <assert.h>
#include <sstream>
#include <poll.h>

namespace mymuduo
{
    const int Channel::kNoneEvent = 0;
    const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
    const int Channel::kWriteEvent = EPOLLOUT;

    Channel::Channel(EventLoop *loop, int fd)
        : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false), addedToLoop_(false)
    {
        LOG_INFO << "channel has been created";
    }
    Channel::~Channel() {}

    void Channel::handleEvent(Timestamp receieveTime)
    {
        if (tied_)
        {
            std::shared_ptr<void> guard = tie_.lock();
            if (guard)
            {
                handleEventWithGuard(receieveTime);
            }
        }
        else
        {
            handleEventWithGuard(receieveTime);
        }
    }

    // [TODO] channel的tie方法什么时候调用？
    void Channel::tie(const std::shared_ptr<void> &obj)
    {
        tie_ = obj;
        tied_ = true;
    }

    void Channel::remove()
    {
        // [TODO] add code...
        assert(isNoneEvent());
        addedToLoop_ = false;
        loop_->removeChannel(this);
    }

    // 当改变channel所表示fd的events事件后，update负责在poller里面更改fd相应的事件epoll_ctl
    // EventLoop => ChannelList Poller
    void Channel::update()
    {
        // 通过channel所属的EventLoop，调用poller的相应方法，注册fd的events事件
        // [TODO] add code...
        addedToLoop_ = true;
        loop_->updateChannel(this);
    }

    // 根据poller通知channel的发生的具体事件，由channel负责调用具体的回调操作
    void Channel::handleEventWithGuard(Timestamp receivetime)
    {
        LOG_FMT_INFO("channel handleEvent revents:%d\n", revents_);

        if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
        {
            if (closeCallback_)
                closeCallback_();
        }
        if (revents_ & EPOLLERR)
        {
            if (errorCallback_)
                errorCallback_();
        }

        if (revents_ & (EPOLLIN | EPOLLPRI))
        {
            if (readCallback_)
                readCallback_(receivetime);
        }
        if (revents_ & EPOLLOUT)
        {
            if (writeCallback_)
                writeCallback_();
        }
    }

    std::string Channel::reventsToString() const
    {
        return eventsToString(fd_, revents_);
    }

    std::string Channel::eventsToString() const
    {
        return eventsToString(fd_, revents_);
    }

    std::string Channel::eventsToString(int fd, int ev)
    {
        std::ostringstream oss;
        oss << fd << ": ";
        if (ev & EPOLLIN)
            oss << "IN ";
        if (ev & EPOLLPRI)
            oss << "PRI ";
        if (ev & EPOLLOUT)
            oss << "OUT ";
        if (ev & EPOLLHUP)
            oss << "HUP ";
        if (ev & EPOLLRDHUP)
            oss << "RDHUP ";
        if (ev & EPOLLERR)
            oss << "ERR ";
        if (ev & POLLNVAL)
            oss << "NVAL ";

        return oss.str();
    }
} // namespace mymuduo