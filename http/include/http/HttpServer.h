#pragma once

#include <mymuduo/TcpServer.h>

namespace http
{
    class HttpRequest;
    class HttpResponse;

    class HttpServer : public mymuduo::noncopyable
    {
    public:
        using HttpCallback = std::function<void(const HttpRequest &, HttpResponse *)>;
        HttpServer(mymuduo::EventLoop *loop,
                   const mymuduo::InetAddress &listenAddr,
                   const std::string &name,
                   mymuduo::TcpServer::Option option = mymuduo::TcpServer::kNoReusePort);

        mymuduo::EventLoop *getLoop() const { return server_.getLoop(); }

        /// Not thread safe, callback be registered before calling start().
        void setHttpCallback(const HttpCallback &cb)
        {
            httpCallback_ = cb;
        }

        void setThreadNum(int numThreads)
        {
            server_.setThreadNum(numThreads);
        }

        void start();

    private:
        void onConnection(const mymuduo::TcpConnectionPtr &conn);
        void onMessage(const mymuduo::TcpConnectionPtr &conn,
                       mymuduo::Buffer *buf,
                       mymuduo::Timestamp receiveTime);
        void onRequest(const mymuduo::TcpConnectionPtr &, const HttpRequest &);

        mymuduo::TcpServer server_;
        HttpCallback httpCallback_;
    };

} // namespace http
