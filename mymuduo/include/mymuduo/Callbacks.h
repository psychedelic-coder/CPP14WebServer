#pragma once

#include "mymuduo/Timestamp.h"

#include <memory>
#include <functional>

namespace mymuduo
{
    class Buffer;
    class TcpConnection;

    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
    using TimerCallback = std::function<void()>;
    using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
    using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
    using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr &, size_t)>;

    // the data has been read to(buf, len)
    // Timestamp = return time of poll/epoll，也就是消息到达的时刻，这个时刻要早于读到数据的时刻(read(2)调用或返回)
    using MessageCallback = std::function<void(const TcpConnectionPtr &, Buffer *, Timestamp)>;
} // namespace mymuduo
