#include "mymuduo/Poller.h"
#include "mymuduo/Channel.h"

namespace mymuduo
{
    Poller::Poller(EventLoop *loop) : ownerLoop_(loop) {}
    Poller::~Poller() = default;
    bool Poller::hasChannel(Channel *channel) const
    {
        auto it = channels_.find(channel->fd());

        return it != channels_.end() && it->second == channel;
    }

} // namespace mymuduo
