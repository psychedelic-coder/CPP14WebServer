#include "mymuduo/EventLoopThread.h"
#include "mymuduo/EventLoop.h"

namespace mymuduo
{
    EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                     const std::string &name)
        : loop_(nullptr), exiting_(false), thread_(std::bind(&EventLoopThread::threadFunc, this), name), mutex_(), cond_(), callback_(cb)
    {
    }

    EventLoopThread::~EventLoopThread()
    {
        exiting_ = true;
        if (!loop_)
        {
            loop_->quit();
            thread_.join();
        }
    }

    EventLoop *EventLoopThread::startLoop()
    {
        thread_.start(); // 启动底层的新线程

        EventLoop *loop = nullptr;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            while (loop_ == nullptr)
            {
                cond_.wait(lock);
            }
            loop = loop_;
        }

        return loop;
    }

    // 在单独的新线程里面运行
    void EventLoopThread::threadFunc()
    {
        // oneloop per thread
        EventLoop loop; // 创建一个独立的eventloop， 和调用该方法的EventLoop的数据成员thread_是一一对应的

        if (callback_)
        {
            callback_(&loop);
        }

        {
            std::unique_lock<std::mutex> lock(mutex_);
            loop_ = &loop;
            cond_.notify_one();
        }

        loop.loop(); // EventLoop loop => poller.poll
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = nullptr;
    }

} // namespace mymuduo
