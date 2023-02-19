#include "mymuduo/Thread.h"
#include "mymuduo/CurrentThread.h"

#include <semaphore.h>

namespace mymuduo
{
    std::atomic_int Thread::numCreated_(0);
    Thread::Thread(ThreadFunc func, const std::string &name)
        : started_(false), joined_(false), tid_(0), func_(std::move(func)), name_(name)
    {
        setDefaultName();
    }

    Thread::~Thread()
    {
        if (started_ && !joined_)
        {
            thread_->detach(); // thread类提供的设置分离线程的方法
        }
    }

    void Thread::start() // 一个Thread对象记录一个新线程的详细信息
    {
        sem_t sem;
        ::sem_init(&sem, false, 0);
        started_ = true;
        // 开启线程专门执行该线程函数
        /*thread_ = std::shared_ptr<std::thread>(new std::thread([&]()
                                                               {
                // 获取线程的tid
                tid_ = CurrentThread::tid();
                sem_post(&sem);
                func_(); }));
                */
        thread_ = std::make_shared<std::thread>([&]()
                                                {
                // 获取线程的tid
                tid_ = CurrentThread::tid();
                ::sem_post(&sem);
                func_(); });
        // 这里必须等待获取上面创建的新线程tid值
        ::sem_wait(&sem);
    }

    void Thread::join()
    {
        joined_ = true;
        thread_->join();
    }

    void Thread::setDefaultName()
    {
        int num = ++numCreated_;
        if (name_.empty())
        {
            char buf[32] = {0};
            snprintf(buf, sizeof buf, "Thread%d", num);
            name_ = buf;
        }
    }

} // namespace mymuduo
