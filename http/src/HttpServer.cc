#include <http/HttpContext.h>
#include <http/HttpServer.h>
#include <http/HttpResponse.h>
#include <http/HttpRequest.h>

#include <mymuduo/Logger.h>
#include <mymuduo/TcpConnection.h>

using namespace mymuduo;

namespace http
{
    namespace detail
    {
        /**
         * @brief 默认的http回调函数
         * 设置响应状态码，响应信息并关闭连接
         */
        void defaultHttpCallback(const HttpRequest &, HttpResponse *resp)
        {
            resp->setStatusCode(HttpResponse::k404NotFound);
            resp->setStatusMessage("Not Found");
            resp->setCloseConnection(true);
        }
    } // namespace detail

    HttpServer::HttpServer(EventLoop *loop,
                           const InetAddress &listenAddr,
                           const std::string &name,
                           TcpServer::Option option)
        : server_(loop, listenAddr, name, option),
          httpCallback_(detail::defaultHttpCallback)
    {
        server_.setConnectionCallback(
            std::bind(&HttpServer::onConnection, this, std::placeholders::_1));
        server_.setMessageCallback(
            std::bind(&HttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    }

    void HttpServer::start()
    {
        LOG_INFO << "HttpServer[" << server_.name()
                 << "] starts listening on " << server_.ipPort();
        server_.start();
    }

    void HttpServer::onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            LOG_INFO << "new Connection arrived";
        }
        else
        {
            LOG_INFO << "Connection closed";
        }
    }

    void HttpServer::onMessage(const TcpConnectionPtr &conn,
                               Buffer *buf,
                               Timestamp receiveTime)
    {
        LOG_INFO << "HttpServer::onMessage";
        std::unique_ptr<HttpContext> context(new HttpContext);

#if 0
    // 打印请求报文
    std::string request = buf->GetBufferAllAsString();
    std::cout << request << std::endl;
#endif

        // 进行状态机解析
        // 错误则发送 BAD REQUEST 半关闭
        if (!context->parseRequest(buf, receiveTime))
        {
            LOG_INFO << "parseRequest failed!";
            conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
            conn->shutdown();
        }

        // 如果成功解析
        if (context->gotAll())
        {
            LOG_INFO << "parseRequest success!";
            onRequest(conn, context->request());
            context->reset();
        }
    }

    void HttpServer::onRequest(const TcpConnectionPtr &conn, const HttpRequest &req)
    {
        const std::string &connection = req.getHeader("Connection");
        
        // 判断是长连接还是短连接
        bool close = connection == "close" ||
                     (req.getVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive");
        HttpResponse response(close);
        httpCallback_(req, &response);
        Buffer buf;
        response.appendToBuffer(&buf);
        conn->send(&buf);
        if (response.closeConnection())
        {
            conn->shutdown();
        }
    }

} // namespace http
