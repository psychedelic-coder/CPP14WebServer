#pragma once

#include "mymuduo/noncopyable.h"
#include "mymuduo/Timestamp.h"

#include <functional>
#include <memory>

namespace mymuduo
{
    class EventLoop;
    // 理清楚 EventLoop channel poller之间的关系 其中EventLoop对应Reactor模型中的Demultiplex
    // channel理解为通道，在muduo中封装了sockfd和其感兴趣的Event,如EPOLLIN EPOLLOUT事件
    // 还绑定了poller返回的具体事件。p.s. 其实就是把epoll的入参和出参给封装成了channel
    class Channel : noncopyable
    {
    public:
        using EventCallback = std::function<void()>;
        using ReadEventCallback = std::function<void(Timestamp)>;

        Channel(EventLoop *loop, int fd);
        ~Channel();

        // fd得到poller通知之后，处理事件的。
        void handleEvent(Timestamp receieveTime);

        // 设置回调函数对象
        void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
        void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
        void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
        void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

        // Tie this channel object to the owner object managed by shared_ptr
        // prevent the owner object being destroyed in handleEvent
        void tie(const std::shared_ptr<void> &);

        int fd() const { return fd_; }
        int events() const { return events_; }
        void set_revents(int revt) { revents_ = revt; } // used by pollers

        // 设置fd相应的事件状态
        void enableReading()
        {
            events_ |= kReadEvent;
            update();
        }
        void disableReading()
        {
            events_ &= ~kReadEvent;
            update();
        }
        void enableWriting()
        {
            events_ |= kWriteEvent;
            update();
        }
        void disableWriting()
        {
            events_ &= ~kWriteEvent;
            update();
        }
        void disableAll()
        {
            events_ &= kNoneEvent;
            update();
        }

        // 查看channel events_状态
        bool isNoneEvent() const { return events_ == kNoneEvent; }
        bool isWriting() const { return events_ & kWriteEvent; }
        bool isReading() const { return events_ & kReadEvent; }

        int index() { return index_; }
        void set_index(int idx) { index_ = idx; }

        // one loop per thread
        EventLoop *ownerLoop() { return loop_; }
        void remove();

    private:
        void update();
        void handleEventWithGuard(Timestamp receivetime);

        static const int kNoneEvent;
        static const int kReadEvent;
        static const int kWriteEvent;

        EventLoop *loop_; // 事件循环
        const int fd_;    // fd, Poller监听的对象（其实是epoll监听的多个fd放到了一个fd中）
        int events_;      // 注册fd感兴趣的事件
        int revents_;     // it's the received event types of epoll
        int index_;       // used by poller

        std::weak_ptr<void> tie_;
        bool tied_;

        // 因为channel能够获取poller fd最终发生的事件revents,所以它负责调用具体事件的回调操作
        ReadEventCallback readCallback_;
        EventCallback writeCallback_;
        EventCallback closeCallback_;
        EventCallback errorCallback_;
    };
} // namespace mymuduo