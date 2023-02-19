#include "mymuduo/TcpServer.h"
#include "mymuduo/EventLoop.h"
#include "mymuduo/TcpConnection.h"
#include "mymuduo/InetAddress.h"

#include <iostream>
#include <functional>
#include <string>
using namespace std;
using namespace mymuduo;
using namespace placeholders;

/* 基于muduo网络库开发服务器程序
   1、组合TcpServer对象
   2、创建EventLoop事件循环对象的指针
   3、明确TcpServer对象构造函数需要什么参数，输出ChatServer的构造函数
   4、在当前服务器类的构造函数中，注册处理连接的回调函数和处理读写事件的回调函数
   5、设置合适的服务端线程数量，muduo库会自己划分IO线程和worker线程
*/
class ChatServer
{
public:
    ChatServer(EventLoop *loop,               // 事件循环
               const InetAddress &listenAddr, // IP+Port
               const string &nameArg)
        : _loop(loop),
          _server(loop, listenAddr, nameArg)
    {
        // 给服务器注册用户连接的创建和断开回调
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
        // 给服务器注册用户读写事件回调
        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3)); // _1占位符
        // 设置服务器端的线程数量   1个IO线程，7个工作线程
        _server.setThreadNum(8);
    }

    // 开启事件循环
    void start()
    {
        _server.start();
    }

private:
    // 专门处理用户的连接创建和断开
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            cout << conn->peerAddress().toIPport() << " -> " << conn->localAddress().toIPport() << " state: online" << endl;
        }
        else
        {
            cout << conn->peerAddress().toIPport() << " -> " << conn->localAddress().toIPport() << " state: offline" << endl;
            conn->shutdown(); // close fd
            //_loop->quit();
        }
    }
    // 专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr &conn, // 连接
                   Buffer *buffer,               // 缓冲区
                   Timestamp time)               // 接收到数据的时间信息
    {
        string buf = buffer->retrieveAllString();
        cout << "recv data:" << buf << "time:" << time.tostring() << endl;
        conn->send(buf);
    }
    EventLoop *_loop;
    TcpServer _server;
};

int main()
{
    EventLoop loop; // 作用类似于创建了一个epoll
    InetAddress addr(6666, "127.0.0.1");
    ChatServer server(&loop, addr, "ChatServer");

    server.start(); // listenfd epoll_ctl->epoll
    loop.loop();    // epoll_wait以阻塞方式等待新用户连接，已连接用户的读写事件等操作

    return 0;
}