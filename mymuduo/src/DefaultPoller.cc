#include "mymuduo/Poller.h"
#include "mymuduo/EpollPoller.h"
#include <mymuduo/Logger.h>

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
            Poller *ret = new EpollPoller(loop);
            LOG_INFO << "new EpollPoller";
            return ret; // 生成epoll的实例
        }
    }

} // namespace mymuduo
