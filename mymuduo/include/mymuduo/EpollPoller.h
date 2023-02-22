#pragma once

#include "mymuduo/Poller.h"
#include "mymuduo/Timestamp.h"

#include <vector>
#include <sys/epoll.h>

/**
 * @brief epoll的使用
 * epoll_create
 * epoll_ctl    add/mod/del
 * epoll_wait
 */
namespace mymuduo
{
    class EpollPoller : public Poller
    {
    public:
        EpollPoller(EventLoop *loop);
        ~EpollPoller() override;

        // 给所有IO复用保留统一的接口
        // 重写抽象基类Poller的抽象方法
        Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
        void updateChannel(Channel *channel) override;
        void removeChannel(Channel *channel) override;
        // 判断参数channel是否在当前poller中
        bool hasChannel(Channel *channel) const override;

    private:
        static const int kInitEventListSize = 16;
        using EventList = std::vector<epoll_event>;

        static const char* operationToString(int op);
        void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
        // 相当于调用epoll_ctl()
        void update(int operation, Channel *channel);

        int epollfd_;
        EventList events_;
    };

} // namespace mymuduo