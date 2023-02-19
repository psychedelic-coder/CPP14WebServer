#include "mymuduo/EventLoop.h"
#include "mymuduo/Acceptor.h"
#include "mymuduo/InetAddress.h"

#include <iostream>
#include <sys/types.h>
#include <unistd.h>

using namespace mymuduo;
// [Test]
void newConnection(int sockfd, const InetAddress &peerAddr)
{
    std::cout << "newConnection(): accepted a new connection from "
              << peerAddr.toIPport() << std::endl;
    ::write(sockfd, "How are you?\n", 13);
    ::close(sockfd);
}

int main()
{
    std::cout << "main(): pid = " << ::getpid() << std::endl;

    InetAddress listenAddr(9981);
    EventLoop loop;

    Acceptor acceptor(&loop, listenAddr, false);
    acceptor.setNewConnectionCallback(newConnection);
    acceptor.listen();

    loop.loop();
}