#pragma once

#include <stdint.h>
#include <memory>

namespace mymuduo
{
    class Timer;
    class TimerId
    {
    public:
        TimerId(std::shared_ptr<Timer> timer, int64_t seq) : timer_(timer), sequence_(seq){};
        friend class TimerQueue;

    private:
        std::shared_ptr<Timer> timer_;
        int64_t sequence_;
    };
} // namespace mymuduo
