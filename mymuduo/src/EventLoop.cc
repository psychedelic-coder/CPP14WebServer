#include "mymuduo/EventLoop.h"
#include "mymuduo/Logger.h"
#include "mymuduo/Poller.h"
#include "mymuduo/Channel.h"
#include "mymuduo/TimerQueue.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

namespace mymuduo
{
    /**
     * @brief 关于__thread关键字
     * The __thread storage class marks a static variable as having thread-local storage duration.
     * This means that in a multi-threaded application a unique instance of the variable
     * is created for each thread that uses it and destroyed when the thread terminates.
     * The __thread storage class specifier can provide a convenient way of assuring thread-safety;
     * declaring an object as per-thread allows multiple threads to access the object without
     * the concern of race conditions while avoiding the need for low-level programming of thread synchronization
     * or significant program restructuring.
     * 防止一个线程创建多个对象
     */
    // 防止一个线程创建多个EventLoop对象
    // 也就是thread-local机制
    __thread EventLoop *t_loopInThisThread = nullptr;
    // 定义默认的Poller IO复用接口的超时时间
    const int kPollTimeMs = 10000;

    // 定义一个全局函数，而不是EventLoop的方法
    // 创建wakeupfd，用来通知并唤醒subReactor
    int createEventfd()
    {
        int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (evtfd < 0)
        {
            LOG_FMT_FATAL("eventfd err!:%d \n", errno);
        }
        return evtfd;
    }

    EventLoop::EventLoop()
        : looping_(false), quit_(false), callingPendingFunctors_(false), threadId_(CurrentThread::tid()), poller_(Poller::newDefaultPoller(this)), wakeupFd_(createEventfd()), wakeupChannel_(new Channel(this, wakeupFd_))
    {
        LOG_FMT_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
        if (t_loopInThisThread)
        {
            LOG_FMT_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
        }
        else
        {
            t_loopInThisThread = this;
        }

        // 设置wakeupfd的事件类型以及事件发生后的回调操作
        wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
        // 每一个eventloop都将监听wakeupchannel的EPOLLIN读事件了
        wakeupChannel_->enableReading();
    }

    EventLoop::~EventLoop()
    {
        wakeupChannel_->disableAll();
        wakeupChannel_->remove();
        ::close(wakeupFd_);
        t_loopInThisThread = nullptr;
    }

    void EventLoop::loop()
    {
        looping_ = true;
        quit_ = false;
        LOG_FMT_INFO("EventLoop %p start looping \n", this);

        while (!quit_)
        {
            activeChannels_.clear();
            // 监听两类fd   一种是client的fd 一种是wakeup的fd
            pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
            for (Channel *channel : activeChannels_)
            {
                // Poller监听哪些channel发生事件了，然后上报给EventLoop
                channel->handleEvent(pollReturnTime_);
            }
            // 执行当前EventLoop事件循环需要处理的回调操作
            /**
             * @brief IO mainloop accept fd<-channel打包 subloop
             * mainloop 事先注册一个回调cd(需要subloop)来执行
             * wakeup subloop后,执行doPendingFunctors()方法，执行之前mainloop注册的cb
             */
            doPendingFunctors();
        }

        LOG_FMT_INFO("EventLoop %p stop looping. \n", this);
        looping_ = false;
    }

    // 退出事件循环 1、loop在自己的线程中调用quit
    void EventLoop::quit()
    {
        quit_ = true;
        // There is a chance that loop() just executes  while(!quit) and exits，
        // then EventLoop destructs, then we are accessing an invalid object,
        // Can be fixed using mutex_ in both places.
        if (!isInLoopThread()) // 2、如果在其他线程中调用quit，   在一个subloop-工作线程中,调用了mainloop-IO线程的quit
        {
            wakeup();
        }
    }

    // 在当前loop中执行cb
    void EventLoop::runInLoop(Functor cb)
    {
        if (isInLoopThread())
        {
            cb();
        }
        else // 在非loop线程中执行cb
        {
            queueInLoop(std::move(cb));
        }
    }

    // 把cb放入队列中，并在必要时唤醒loop所在的线程(也就是陈硕说的tmd的IO线程)
    void EventLoop::queueInLoop(Functor cb)
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            pendingFunctors_.emplace_back(cb);
        }

        // 唤醒相应的需要执行cb执行的loop
        if (!isInLoopThread() || callingPendingFunctors_)
        {
            wakeup();
        }
    }

    // 唤醒loop所在线程，向wakeupFd_写一个数据，wakeupchannel就发生读事件，loop就会被唤醒
    void EventLoop::wakeup()
    {
        uint64_t one = 1;
        ssize_t n = write(wakeupFd_, &one, sizeof one);
        if (n != sizeof one)
        {
            LOG_FMT_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
        }
    }

    TimerId EventLoop::runAt(Timestamp time, TimerCallback cb)
    {
        return timerQueue_->addTimer(cb, time, 0.0);
    }

    TimerId EventLoop::runAfter(double delay, TimerCallback cb)
    {
        Timestamp time(addTime(Timestamp::now(), delay));
        return runAt(time, cb);
    }

    TimerId EventLoop::runEvery(double interval, TimerCallback cb)
    {
        Timestamp time(addTime(Timestamp::now(), interval));
        return timerQueue_->addTimer(cb, time, interval);
    }

    void EventLoop::updateChannel(Channel *channel)
    {
        poller_->updateChannel(channel);
    }

    void EventLoop::removeChannel(Channel *channel)
    {
        poller_->removeChannel(channel);
    }

    bool EventLoop::hasChannel(Channel *channel)
    {
        return poller_->hasChannel(channel);
    }

    void EventLoop::abortNotInLoopThread()
    {
        LOG_FMT_FATAL("EventLoop::abortNotInLoopThread - EventLoop %p was created in threadId_ = %d, current thread id = %d", this, threadId_, CurrentThread::tid());
    }

    void EventLoop::handleRead()
    {
        uint64_t one = 1;
        int n = ::read(wakeupFd_, &one, sizeof one);
        if (n != sizeof one)
        {
            LOG_FMT_ERROR("EventLoop::handleRead() reads %d bytes instead of 8", n);
        }
    }

    // 执行回调
    void EventLoop::doPendingFunctors()
    {
        std::vector<Functor> functors;
        callingPendingFunctors_ = true;

        {
            std::unique_lock<std::mutex> lock(mutex_);
            functors.swap(pendingFunctors_);
        }

        for (const Functor &functor : functors)
        {
            functor(); // 执行当前loop需要执行的回调操作
        }

        callingPendingFunctors_ = false;
    }
} // namespace mymuduo
