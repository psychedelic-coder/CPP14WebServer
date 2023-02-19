#pragma once

#include "mymuduo/noncopyable.h"
#include "mymuduo/Thread.h"
#include "mymuduo/LogStream.h"

#include <atomic>
#include <string>
#include <mutex>
#include <condition_variable>
#include <vector>
namespace mymuduo
{
    class AsyncLogging : noncopyable
    {
    public:
        AsyncLogging(const std::string basename, int flushInterval = 2);
        ~AsyncLogging()
        {
            if (running_)
                stop();
        }

        void append(const char *logline, int len);

        void start()
        {
            running_ = true;
            thread_.start();
        }

        void stop()
        {
            running_ = false;
            cond_.notify_one();
            thread_.join();
        }

    private:
        void threadFunc();

        using Buffer = mymuduo::detail::FixedBuffer<mymuduo::detail::kLargeBuffer>;
        using BufferVector = std::vector<std::unique_ptr<Buffer>>;
        using BufferPtr = BufferVector::value_type;

        const int flushInterval_;
        std::atomic_bool running_;
        std::string basename_;
        Thread thread_;
        std::mutex mutex_;
        std::condition_variable cond_;

        BufferPtr currentBuffer_;
        BufferPtr nextBuffer_;
        BufferVector buffers_;
    };
} // namespace mymuduo
