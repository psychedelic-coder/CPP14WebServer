#pragma once

#include "mymuduo/Callbacks.h"
#include "mymuduo/Timestamp.h"
#include "mymuduo/noncopyable.h"

#include <atomic>

namespace mymuduo
{
    /// @brief Internal class for timer event
    class Timer : noncopyable
    {
    public:
        Timer(TimerCallback cb, Timestamp when, double interval)
            : callback_(std::move(cb)),
              expiration_(when),
              interval_(interval),
              repeat_(interval > 0.0),
              sequence_(++s_numCreated_)
        {
        }

        ~Timer()
        {
        }
        void run() const
        {
            callback_();
        }

        Timestamp expiration() const { return expiration_; }
        bool repeat() const { return repeat_; }
        int64_t sequence() const { return sequence_; }

        void restart(Timestamp now);

        static int64_t numCreated() { return s_numCreated_; }

    private:
        const TimerCallback callback_;
        Timestamp expiration_;
        const double interval_;
        const bool repeat_;
        const int64_t sequence_;

        static std::atomic_int64_t s_numCreated_;
    };
} // namespace mymuduo
