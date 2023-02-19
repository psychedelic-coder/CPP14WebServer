# CPP14WebServer

## 项目介绍

本项目是参考 muduo 实现的基于 Reactor 模型的多线程网络库。使用 C++ 14 编写去除 muduo 对 boost 的依赖，内部实现了一个小型的 HTTP 服务器，可支持 GET 请求和静态资源的访问，且附有异步日志监控服务端情况。

项目实现了 Channel 模块、Poller 模块、事件循环模块、HTTP 模块、定时器模块、异步日志模块。

## 项目特点

- 基于C++14对muduo网络库的重构，去除对boost库的依赖 

- 基于网络库构建了http服务器，使用状态机解析了HTTP请求,支持管线化
- 基于自实现的双缓冲区实现异步日志，由后端线程负责定时向磁盘写入前端日志信息，避免数据落盘时阻塞网络服务
- 支持流式日志风格写日志和格式化风格写日志。流式日志使用LOG_INFO << "this is a log in stream type."; 格式化日志使用LOG_FMT("TcpConnection::ctor[%s] at fd=%d \n", name_.c_str(), sockfd)
- 基于红黑树实现定时器管理结构，内部使用 Linux 的 timerfd 通知到期任务，高效管理定时任务。
- 底层使用 Epoll + LT 模式的 I/O 复用模型，并且结合非阻塞 I/O 实现主从 Reactor 模型
- 采用「one loop per thread」线程模型，使用多线程充分利用多核CPU并向上封装线程池避免线程创建和销毁带来的性能开销
- 主线程只负责accept请求，并以Round Robin的方式分发给其它IO线程(兼计算线程)，锁的争用只会出现在主线程和某一特定线程中
- 使用eventfd实现了线程的异步唤醒
- 为减少内存泄漏的可能，使用make_shared和C++14开始支持的make_unique消灭掉所有的new，将动态对象交给智能指针管理
- 支持优雅关闭连接

## 开发环境

- 操作系统：Ubuntu 20.04.5 LTS
- 编译器：clang version 10.0.0
- 编辑器：vscode
- 版本控制：git
- 项目构建：cmake version 3.25.0

## 感谢
- 《Linux高性能服务器编程》

- 《Linux多线程服务端编程：使用muduo C++网络库》
