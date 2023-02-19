#include "mymuduo/Poller.h"
#include "mymuduo/EpollPoller.h"

#include <stdlib.h>

namespace mymuduo
{
    Poller *Poller::newDefaultPoller(EventLoop *loop)
    {
        if (::getenv("MUDUO_USE_POLL"))
        {
            return nullptr; // 生成poll的实例
        }
        else
        {
            return new EpollPoller(loop); // 生成epoll的实例
        }
    }

} // namespace mymuduo
