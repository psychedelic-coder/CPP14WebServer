#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

namespace mymuduo
{

    // 封装socket地址类型
    class InetAddress
    {
    public:
        explicit InetAddress(uint16_t port = 6666, std::string ip = "127.0.0.1");
        explicit InetAddress(const sockaddr_in &addr)
            : addr_(addr){};
        std::string toIP() const;
        std::string toIPport() const;
        uint16_t toPort() const;

        const sockaddr_in *getSockAddr() const { return &addr_; }
        void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }

    private:
        sockaddr_in addr_;
    };

} // namespace mymuduo