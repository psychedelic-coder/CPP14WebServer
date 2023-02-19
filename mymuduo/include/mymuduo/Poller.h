#pragma once

#include "mymuduo/noncopyable.h"
#include "mymuduo/Timestamp.h"

#include <vector>
#include <unordered_map>

namespace mymuduo
{
    class Channel;
    class EventLoop;
    // muduo库中IO复用的封装！Poller是个抽象基类。
    class Poller : noncopyable
    {
    public:
        using ChannelList = std::vector<Channel *>;

        Poller(EventLoop *loop);
        virtual ~Poller();
        // 给所有IO复用保留统一的接口
        virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
        virtual void updateChannel(Channel *channel) = 0;
        // Poller并不拥有Channel，Channel在Poller析构之前必须先unregister，避免空悬指针。
        virtual void removeChannel(Channel *channel) = 0;
        // 判断参数channel是否在当前poller中
        virtual bool hasChannel(Channel *channel) const;

        // EvemtLoop可以通过该接口获取默认的IO复用的具体实现
        static Poller *newDefaultPoller(EventLoop *loop);

    protected:
        // map的key：sockfd, value: sockfd所属的channel
        using ChannelMap = std::unordered_map<int, Channel *>;
        ChannelMap channels_;

    private:
        EventLoop *ownerLoop_; // 定义poller所属的事件循环
    };
} // mymuduo