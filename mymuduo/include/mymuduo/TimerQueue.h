#pragma once

#include "mymuduo/noncopyable.h"
#include "mymuduo/Callbacks.h"
#include "mymuduo/Channel.h"

#include <set>
#include <vector>
#include <memory>
#include <atomic>

namespace mymuduo
{
    class EventLoop;
    class TimerId;
    class Timer;

    class TimerQueue : noncopyable
    {
    public:
        explicit TimerQueue(EventLoop *loop);
        ~TimerQueue();
        ///
        /// Schedules the callback to be run at given time,
        /// repeats if @c interval > 0.0.
        ///
        /// Must be thread safe. Usually be called from other threads.
        TimerId addTimer(const TimerCallback &cb, Timestamp when, double interval);

        void cancel(TimerId timerId);

    private:
        using Entry = std::pair<Timestamp, std::shared_ptr<Timer>>;
        using TimerSet = std::set<Entry>;

        using ActiveTimer = std::pair<std::shared_ptr<Timer>, int64_t>;
        using ActiveTimerSet = std::set<ActiveTimer>;

        // 借助Event::Loop::runInLoop()将addTimer做成线程安全的而且无须用锁。
        // 具体做法是让addTimer()调用runInLoop()，把实际工作转移到IO线程来做。
        void addTimerInLoop(std::shared_ptr<Timer> timer);
        void cancelInLoop(TimerId timerId);
        // called when timerfd alarms
        void handleRead();
        // move out all expiered timers
        std::vector<Entry> getExpired(Timestamp now);
        void reset(const std::vector<Entry> &expired, Timestamp now);

        bool insert(std::shared_ptr<Timer> timer);

        EventLoop *loop_;
        // TimerQueue用Timer的Channel的ReadCallback来读timerfd
        const int timerfd_;
        Channel timerfdChannel_;
        // Timer set sorted by expiration
        TimerSet timers_;

        //for getExpired
        std::shared_ptr<Timer> timerEntryPtr_;
        
        // for cancel()
        ActiveTimerSet activeTimers_;
        std::atomic<bool> callingExpiredTimers_; /* atomic */
        ActiveTimerSet cancelingTimers_;
    };

} // namespace mymuduo
