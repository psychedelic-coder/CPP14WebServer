#include "mymuduo/Logger.h"
#include "mymuduo/Timestamp.h"
#include "mymuduo/CurrentThread.h"

#include <iostream>

namespace mymuduo
{

    /*/ 获取日志唯一的实例对象
    Logger &Logger::instance()
    {
        static Logger Logger;
        return Logger;
    }

    // 设置日志级别
    void Logger::setLoglevel(int level)
    {
        logLevel_ = level;
    }

    // 写日志 [级别信息] time : msg
    void LOG_FMTger::LOG_FMT(std::string msg)
    {
        switch (LOG_FMTLevel_)
        {
        case INFO:
            std::cout << "[INFO]";
            break;
        case ERROR:
            std::cout << "[ERROR]";
            break;
        case FATAL:
            std::cout << "[FATAL]";
            break;
        case DEBUG:
            std::cout << "[DEBUG]";
            break;
        default:
            break;
        }

        // 打印时间和msg
        // [TODO]:完成Timestamp类打印时间
        std::cout << mymuduo::Timestamp::now().tostring() << std::endl;
    }*/

    __thread char t_errnobuf[512];
    __thread char t_time[64];
    __thread time_t t_lastSecond;

    const char *getErrnoMsg(int savedErrno)
    {
        return ::strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
    }

    Logger::LogLevel initLogLevel()
    {
        return Logger::INFO;
    }

    Logger::LogLevel g_logLevel = initLogLevel();

    // NUM_LOG_LEVELS = 5
    const char *LogLevelName[5] =
        {
            "DEBUG ",
            "INFO  ",
            "WARN  ",
            "ERROR ",
            "FATAL "};

    // helper class for known string length at compile time
    class T
    {
    public:
        T(const char *str, unsigned len)
            : str_(str),
              len_(len)
        {
        }
        T(const std::string str) : str_(str.data()), len_(str.size())
        {
        }
        const char *str_;
        const unsigned len_;
    };

    inline LogStream &operator<<(LogStream &s, T v)
    {
        s.append(v.str_, v.len_);
        return s;
    }

    inline LogStream &operator<<(LogStream &s, const Logger::SourceFile &v)
    {
        s.append(v.data_, v.size_);
        return s;
    }

    void defaultOutput(const char *msg, int len)
    {
        size_t n = ::fwrite(msg, 1, len, stdout);
        // FIXME check n
        (void)n;
    }

    void defaultFlush()
    {
        ::fflush(stdout);
    }

    Logger::OutputFunc g_output = defaultOutput;
    Logger::FlushFunc g_flush = defaultFlush;
} // namespace mymuduo

// Logger成员函数
namespace mymuduo
{
    Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile &file, int line)
        : time_(Timestamp::now()),
          stream_(),
          level_(level),
          line_(line),
          basename_(file)
    {
        formatTime();
        CurrentThread::tid();
        stream_ << T(std::to_string(CurrentThread::tid()));
        stream_ << T(LogLevelName[level], 6);
        if (savedErrno != 0)
        {
            stream_ << getErrnoMsg(savedErrno) << " (errno=" << savedErrno << ") ";
        }
    }

    void Logger::Impl::formatTime()
    {
        Timestamp now = Timestamp::now();
        time_t seconds = static_cast<time_t>(now.microSecondsSinceEpoch() / Timestamp::kMicroSecondsPerSecond);
        int microseconds = static_cast<int>(now.microSecondsSinceEpoch() % Timestamp::kMicroSecondsPerSecond);

        struct tm *tm_time = ::localtime(&seconds);
        // 写入此线程存储的时间buf中
        snprintf(t_time, sizeof(t_time), "%4d/%02d/%02d %02d:%02d:%02d",
                 tm_time->tm_year + 1900,
                 tm_time->tm_mon + 1,
                 tm_time->tm_mday,
                 tm_time->tm_hour,
                 tm_time->tm_min,
                 tm_time->tm_sec);
        // 更新最后一次时间调用
        t_lastSecond = seconds;

        // muduo使用Fmt格式化整数，这里我们直接写入buf
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "%06d ", microseconds);

        // 输出时间，附有微秒(之前是(buf, 6),少了一个空格)
        stream_ << T(t_time, 17) << T(buf, 7);
    }

    void Logger::Impl::finish()
    {
        stream_ << " - " << basename_ << ':' << line_ << '\n';
    }

    // Timestamp::toString方法的思路，只不过这里需要输出到流
    Logger::Logger(SourceFile file, int line)
        : impl_(INFO, 0, file, line)
    {
    }

    Logger::Logger(SourceFile file, int line, LogLevel level)
        : impl_(level, 0, file, line)
    {
    }

    // 可以打印调用函数
    Logger::Logger(SourceFile file, int line, LogLevel level, const char *func)
        : impl_(level, 0, file, line)
    {
        impl_.stream_ << func << ' ';
    }

    Logger::~Logger()
    {
        impl_.finish();
        // 获取buffer
        const LogStream::Buffer &buf(stream().buffer());
        // 输出(默认向终端输出)
        g_output(buf.data(), buf.length());
        // FATAL情况终止程序
        if (impl_.level_ == FATAL)
        {
            g_flush();
            abort();
        }
    }

    void Logger::setLogLevel(LogLevel level)
    {
        g_logLevel = level;
    }

    void Logger::setOutput(OutputFunc out)
    {
        g_output = out;
    }

    void Logger::setFlush(FlushFunc flush)
    {
        g_flush = flush;
    }
}