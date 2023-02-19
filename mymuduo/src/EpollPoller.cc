#include "mymuduo/EpollPoller.h"
#include "mymuduo/Logger.h"
#include "mymuduo/Channel.h"

#include <errno.h>
#include <unistd.h>
#include <string.h>

namespace mymuduo
{
    const int kNew = -1;    // Channel的成员index_ = -1,表示channel object未添加到Poller中
    const int kAdded = 1;   // 已添加
    const int kDeleted = 2; // 已删除

    EpollPoller::EpollPoller(EventLoop *loop)
        : Poller(loop), epollfd_(::epoll_create1(EPOLL_CLOEXEC)), events_(kInitEventListSize)
    {
        if (epollfd_ < 0)
        {
            LOG_FMT_FATAL("epoll_create error: %d", errno);
        }
    }

    EpollPoller::~EpollPoller()
    {
        ::close(epollfd_);
    }

    Timestamp EpollPoller::poll(int timeoutMs, ChannelList *activeChannels)
    {
        // 实际上应该用LOG_FMT_DEBUG输出日志更为合理 
        LOG_FMT_DEBUG("func=%s => fd total count:%lu\n", channels_.size());

        int numEvents = ::epoll_wait(epollfd_, &(*events_.begin()), static_cast<int>(events_.size()), timeoutMs);
        int saveError = errno;
        Timestamp now(Timestamp::now());

        if (numEvents > 0)
        {
            LOG_FMT_DEBUG("%d events happened \n", numEvents);
            fillActiveChannels(numEvents, activeChannels);
            if (numEvents == events_.size())
            {
                // 对events_扩容
                events_.resize(events_.size() * 2);
            }
        }
        else if (numEvents == 0)
        {
            LOG_FMT_DEBUG("%s timeout!\n", __FUNCTION__);
        }
        else
        {
            if (saveError != EINTR)
            {
                errno = saveError;
                LOG_FMT_ERROR("Epoller::poll() err!");
            }
        }

        return now;
    }

    // channel update remove => EventLoop updateChannel removeChannel => Poller
    /**
     *              EventLoop
     *      ChannelList     Poller
     *                        |
     *                     ChannelMap<fd, Channel*>
     * channel
     */
    void EpollPoller::updateChannel(Channel *channel)
    {
        const int index = channel->index();
        LOG_FMT_INFO("func = %s => fd = %d events = %d index = %d \n", __FUNCTION__, channel->fd(), channel->events(), index);

        if (index == kNew || index == kDeleted)
        {
            int fd = channel->fd();
            if (index == kNew)
            {
                channels_[fd] = channel;
            }

            channel->set_index(kAdded);
            update(EPOLL_CTL_ADD, channel);
        }
        else
        {
            int fd = channel->fd();
            if (channel->isNoneEvent())
            {
                update(EPOLL_CTL_DEL, channel);
                channel->set_index(kDeleted);
            }
            else
            {
                update(EPOLL_CTL_MOD, channel);
            }
        }
    }
    /**
     * @brief 从poller中删除channel
     *
     * @param channel
     */
    void EpollPoller::removeChannel(Channel *channel)
    {
        int fd = channel->fd();
        channels_.erase(fd);

        LOG_FMT_INFO("func = %s => fd = %d\n", __FUNCTION__, fd);

        int index = channel->index();
        if (index == kAdded)
        {
            update(EPOLL_CTL_DEL, channel);
        }
        channel->set_index(kNew);
    }

    bool EpollPoller::hasChannel(Channel *channel) const
    {
        ChannelMap::const_iterator it = channels_.find(channel->fd());
        return it != channels_.end() && it->second == channel;
    }

    void EpollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
    {
        for (int i = 0; i < numEvents; ++i)
        {
            Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
            channel->set_revents(events_[i].events);
            activeChannels->push_back(channel);
        }
    }

    /**
     * @brief 相当于epoll_ctl add/mod/del
     *
     * @param operation
     * @param channel
     */
    void EpollPoller::update(int operation, Channel *channel)
    {
        epoll_event event;
        memset(&event, 0, sizeof event);
        int fd = channel->fd();
        event.events = channel->events();
        event.data.ptr = channel;
        event.data.fd = fd;

        if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
        {
            if (operation == EPOLL_CTL_DEL)
            {
                LOG_FMT_ERROR("epoll_ctl del error: %d\n", errno);
            }
            else
            {
                LOG_FMT_FATAL("epoll_ctl add / mod error: %d\n", errno);
            }
        }
    }

} // namespace mymuduo
