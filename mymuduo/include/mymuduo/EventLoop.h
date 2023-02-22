#pragma once

#include "mymuduo/noncopyable.h"
#include "mymuduo/CurrentThread.h"
#include "mymuduo/Timestamp.h"
#include "mymuduo/TimerId.h"
#include "mymuduo/Callbacks.h"

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

namespace mymuduo
{
    class Channel;
    class Poller;
    class TimerQueue;
    /**
     * @brief 事件循环类，对应Reactor模型中的Demultiplex。里面主要包含了两个大模块 Channel Poller(epoll的抽象)
     * 理清楚 EventLoop channel poller之间的关系 其中EventLoop对应Reactor模型中的Demultiplex
     * channel理解为通道，在muduo中封装了sockfd和其感兴趣的Event,如EPOLLIN EPOLLOUT事件
     * 还绑定了poller返回的具体事件。p.s. 其实就是把epoll的入参和出参给封装成了channel
     */
    class EventLoop
    {
    public:
        using Functor = std::function<void()>;

        EventLoop();
        ~EventLoop();

        void loop();
        void quit();

        Timestamp pollReturnTime() const { return pollReturnTime_; }

        void runInLoop(Functor cb);
        void queueInLoop(Functor cb);

        // timers

        ///
        /// Runs callback at 'time'.
        /// Safe to call from other threads.
        ///
        TimerId runAt(Timestamp time, TimerCallback cb);
        ///
        /// Runs callback after @c delay seconds.
        /// Safe to call from other threads.
        ///
        TimerId runAfter(double delay, TimerCallback cb);
        ///
        /// Runs callback every @c interval seconds.
        /// Safe to call from other threads.
        ///
        TimerId runEvery(double interval, TimerCallback cb);
        ///
        /// Cancels the timer.
        /// Safe to call from other threads.
        ///
        // void cancel(TimerId timerId);

        void wakeup();

        void updateChannel(Channel *channel);
        void removeChannel(Channel *channel);
        bool hasChannel(Channel *channel);

        // pid_t threadId() const { return threadId_; }
        void assertInLoopThread()
        {
            if (!isInLoopThread())
            {
                abortNotInLoopThread();
            }
        }

        bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

    private:
        void abortNotInLoopThread();
        void handleRead();
        void doPendingFunctors();
        using ChannelList = std::vector<Channel *>;

        std::atomic_bool looping_; // 原子操作，通过CAS实现的
        std::atomic_bool quit_;    // 标识退出bool循环

        const pid_t threadId_;     // 记录当前loop所在线程的ID
        Timestamp pollReturnTime_; // poller返回发生事件的Channels的时间点
        std::unique_ptr<Poller> poller_;

        /// timer
        std::unique_ptr<TimerQueue> timerQueue_;

        int wakeupFd_; // 主要作用，当mainloop获取一个新用户的channel，通过轮询算法选择一个subloop，通过该成员唤醒subloop处理channel
        std::unique_ptr<Channel> wakeupChannel_;

        ChannelList activeChannels_;
        Channel *currentActiveChannel_;

        std::atomic_bool callingPendingFunctors_; // 标识当前loop是否有需要执行的回调操作
        std::vector<Functor> pendingFunctors_;    // 存储loop需要执行的所有的回调操作
        std::mutex mutex_;                        // 互斥锁，用来保护上面vector容器的线程安全操作
    };

} // namespace mymuduo