#include "mymuduo/TimerQueue.h"
#include "mymuduo/TimerId.h"
#include "mymuduo/Timer.h"
#include "mymuduo/EventLoop.h"
#include "mymuduo/Logger.h"
#include "mymuduo/Timestamp.h"

#include <sys/timerfd.h>
#include <string.h>

namespace mymuduo
{
    namespace detail
    {
        int createTimerfd()
        {
            int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

            if (timerfd < 0)
            {
                LOG_FMT_FATAL("Failed in timerfd_create");
            }

            return timerfd;
        }

        struct timespec howMuchTimeFromNow(Timestamp when)
        {
            int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
            if (microseconds < 100)
            {
                microseconds = 100;
            }
            struct timespec ts;
            ts.tv_sec = static_cast<time_t>(
                microseconds / Timestamp::kMicroSecondsPerSecond);
            ts.tv_nsec = static_cast<long>(
                (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
            return ts;
        }

        void readTimerfd(int timerfd, Timestamp now)
        {
            uint64_t howmany;
            ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
            if (n != sizeof howmany)
            {
                LOG_FMT_ERROR("TimerQueue::handleRead() reads %ld bytes instead of 8", n);
            }
        }

        void resetTimerfd(int timerfd, Timestamp expiration)
        {
            // wake up loop by timerfd_settime()
            struct itimerspec newValue;
            struct itimerspec oldValue;
            memset(&newValue, 0, sizeof newValue);
            memset(&oldValue, 0, sizeof oldValue);
            newValue.it_value = howMuchTimeFromNow(expiration);
            int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
            if (ret)
            {
                LOG_FMT_ERROR("timerfd_settime()");
            }
        }

    } // namespace detail

    std::shared_ptr<Timer> timerEntryPtr_(reinterpret_cast<Timer *>(UINTPTR_MAX));

    TimerQueue::TimerQueue(EventLoop *loop)
        : loop_(loop), timerfd_(detail::createTimerfd()), timerfdChannel_(loop_, timerfd_), timers_(), callingExpiredTimers_(false)
    {
        timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
        // we are always reading the timerfd, we disarm it with timerfd_settime.
        timerfdChannel_.enableReading();
    }

    TimerQueue::~TimerQueue()
    {
        // do not remove channel, since we're in EventLoop::dtor();
        timerfdChannel_.disableAll();
        timerfdChannel_.remove();
        ::close(timerfd_);
    }

    TimerId TimerQueue::addTimer(const TimerCallback &cb, Timestamp when, double interval)
    {
        // 消灭掉所有的new
        // std::shared_ptr<Timer> timer(new Timer(std::move(cb), when, interval));
        std::shared_ptr<Timer> timer = std::make_shared<Timer>(std::move(cb), when, interval);
        // Timer *timer = new Timer(std::move(cb), when, interval);
        loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));

        return TimerId(timer, timer->sequence());
    }

    void TimerQueue::cancel(TimerId timerId)
    {
        loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
    }

    void TimerQueue::addTimerInLoop(std::shared_ptr<Timer> timer)
    {
        loop_->assertInLoopThread();
        bool earliestChanged = insert(timer);

        if (earliestChanged)
        {
            // TODO 补充方法定义
            detail::resetTimerfd(timerfd_, timer->expiration());
        }
    }

    void TimerQueue::cancelInLoop(TimerId timerId)
    {
        loop_->assertInLoopThread();
        // assert(timers_.size() == activeTimers_.size());
        ActiveTimer timer(timerId.timer_, timerId.sequence_);
        ActiveTimerSet::iterator it = activeTimers_.find(timer);
        if(it != activeTimers_.end())
        {
            size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
            activeTimers_.erase(it);
        }
        else if(callingExpiredTimers_)
        {
            cancelingTimers_.insert(timer);
        }
        // assert(timers_.size() == activeTimers_.size())
    }
    void TimerQueue::handleRead()
    {
        loop_->assertInLoopThread();
        Timestamp now(Timestamp::now());
        detail::readTimerfd(timerfd_, now);

        std::vector<Entry> expired = getExpired(now);

        callingExpiredTimers_ = true;
        cancelingTimers_.clear();
        // safe to callback outside critical section
        for (const Entry &it : expired)
        {
            it.second->run();
        }
        callingExpiredTimers_ = false;

        reset(expired, now);
    }

    // [TODO]分析
    // move out all expiered timers
    std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
    {
        std::vector<Entry> expired;
        Entry sentry(now, std::shared_ptr<Timer>(timerEntryPtr_));
        TimerSet::iterator end = timers_.lower_bound(sentry);
        std::copy(timers_.begin(), end, back_inserter(expired));
        timers_.erase(timers_.begin(), end);

        for (const Entry &it : expired)
        {
            ActiveTimer timer(it.second, it.second->sequence());
            size_t n = activeTimers_.erase(timer);
        }

        return expired;
    }

    void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now)
    {
        Timestamp nextExpire;

        for (const Entry &it : expired)
        {
            ActiveTimer timer(it.second, it.second->sequence());
            if (it.second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end())
            {
                it.second->restart(now);
                insert(it.second);
            }

            if (!timers_.empty())
            {
                nextExpire = timers_.begin()->second->expiration();
            }

            if (nextExpire.valid())
            {
                detail::resetTimerfd(timerfd_, nextExpire);
            }
        }
    }

    bool TimerQueue::insert(std::shared_ptr<Timer> timer)
    {
        loop_->assertInLoopThread();
        bool earliestChanged = false;
        Timestamp when = timer->expiration();
        TimerSet::iterator it = timers_.begin();
        if(it == timers_.end() || when < it->first)
        {
            earliestChanged = true;
        }
        {
            std::pair<TimerSet::iterator, bool> result = timers_.insert(Entry(when, timer));
        }

        return earliestChanged;
    }

} // namespace mymuduo
