#pragma once

#include "mymuduo/Timestamp.h"
#include "mymuduo/LogStream.h"

#include <string>
#include <string.h>
#include <functional>

// LOG_INFO("%s %d", arg1, arg2)
#define LOG_FMT_INFO(logmsgFormat, ...)                                                           \
    do                                                                                            \
    {                                                                                             \
        char buf[1024] = {0};                                                                     \
        int cnt = snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);                               \
        if (cnt >= 0 && cnt <= 1024)                                                              \
            mymuduo::Logger(__FILE__, __LINE__, mymuduo::Logger::INFO).stream().append(buf, cnt); \
    } while (0)

#define LOG_FMT_ERROR(logmsgFormat, ...)                                                           \
    do                                                                                             \
    {                                                                                              \
        char buf[1024] = {0};                                                                      \
        int cnt = snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);                                \
        if (cnt >= 0 && cnt <= 1024)                                                               \
            mymuduo::Logger(__FILE__, __LINE__, mymuduo::Logger::ERROR).stream().append(buf, cnt); \
    } while (0)

#define LOG_FMT_FATAL(logmsgFormat, ...)                                                           \
    do                                                                                             \
    {                                                                                              \
        char buf[1024] = {0};                                                                      \
        int cnt = snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);                                \
        if (cnt >= 0 && cnt <= 1024)                                                               \
            mymuduo::Logger(__FILE__, __LINE__, mymuduo::Logger::FATAL).stream().append(buf, cnt); \
    } while (0)

#ifdef FMT_DEBUG
#define LOG_FMT_DEBUG(logmsgFormat, ...)                                                           \
    do                                                                                             \
    {                                                                                              \
        char buf[1024] = {0};                                                                      \
        int cnt = snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);                                \
        if (ccnt >= 0 && cnt <= 1024)                                                              \
            mymuduo::Logger(__FILE__, __LINE__, mymuduo::Logger::DEBUG).stream().append(buf, cnt); \
    } while (0)
#else
#define LOG_FMT_DEBUG(logmsgFormat, ...)
#endif

/*#define LOG_FMT_INFO(logmsgFormat, ...)                   \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLoglevel(INFO);                         \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)

#define LOG_FMT_ERROR(logmsgFormat, ...)                  \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLoglevel(ERROR);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)

#define LOG_FMT_FATAL(logmsgFormat, ...)                  \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLoglevel(FATAL);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
        exit(-1);                                         \
    } while (0)

#ifdef MYDEBUG
#define LOG_FMT_DEBUG(logmsgFormat, ...)                  \
    do                                                    \
    {                                                     \
        Logger &logger = Logger::instance();              \
        logger.setLoglevel(DEBUG);                        \
        char buf[1024] = {0};                             \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf);                                  \
    } while (0)
#else
#define LOG_DEBUG(logmsgFormat, ...)
#endif*/

// ????????????????????? INFO ERROR FATAL DEBUG
namespace mymuduo
{
    class Logger
    {
    public:
        // SourceFile???????????????????????????
        class SourceFile
        {
        public:
            template <int N>
            SourceFile(const char (&arr)[N])
                : data_(arr),
                  size_(N - 1)
            {
                const char *slash = ::strrchr(data_, '/'); // builtin function
                if (slash)
                {
                    data_ = slash + 1;
                    size_ -= static_cast<int>(data_ - arr);
                }
            }

            explicit SourceFile(const char *filename)
                : data_(filename)
            {
                /**
                 *      char *strrchr(const char *s, int c);
                 * The strrchr() function returns a pointer to the last occurrence of the character c in the string s.
                 */
                const char *slash = ::strrchr(filename, '/');
                if (slash)
                {
                    data_ = slash + 1;
                }
                size_ = static_cast<int>(strlen(data_));
            }

            const char *data_;
            int size_;
        };

        enum LogLevel
        {
            INFO,  // ????????????
            ERROR, // ????????????
            FATAL, // core??????
            DEBUG  // ????????????
        };

        Logger(SourceFile file, int line);
        Logger(SourceFile file, int line, LogLevel level);
        Logger(SourceFile file, int line, LogLevel level, const char *func);
        ~Logger();

        // ??????????????????
        LogStream &stream() { return impl_.stream_; }

        static LogLevel logLevel();
        static void setLogLevel(LogLevel level);
        // ?????????????????????????????????
        // static Logger &instance();
        // ??????????????????
        // void setLoglevel(int level);
        // ?????????
        // void log(std::string msg);

        using OutputFunc = std::function<void(const char *msg, int len)>;
        using FlushFunc = std::function<void()>;
        static void setOutput(OutputFunc);
        static void setFlush(FlushFunc);

    private:
        class Impl
        {
        public:
            using LogLevel = Logger::LogLevel;
            Impl(LogLevel level, int old_errno, const SourceFile &file, int line);
            void formatTime();
            void finish();

            Timestamp time_;
            LogStream stream_;
            LogLevel level_;
            int line_;
            SourceFile basename_;
        };

        Impl impl_;
    };

    extern Logger::LogLevel g_logLevel;

    inline Logger::LogLevel Logger::logLevel()
    {
        return g_logLevel;
    }

    //
    // CAUTION: do not write:
    //
    // if (good)
    //   LOG_INFO << "Good news";
    // else
    //   LOG_WARN << "Bad news";
    //
    // this expends to
    // g_output
    // if (good)
    //   if (logging_INFO)
    //     logInfoStream << "Good news";
    //   else
    //     logWarnStream << "Bad news";
    //
    // ??????errno??????
    const char *getErrnoMsg(int savedErrno);

/**
 * ?????????????????????????????????????????????
 * ?????????????????????FATAL??????logLevel????????????DEBUG???INFO???DEBUG???INFO??????????????????????????????
 */
#define LOG_DEBUG                                              \
    if (mymuduo::Logger::logLevel() <= mymuduo::Logger::DEBUG) \
    mymuduo::Logger(__FILE__, __LINE__, mymuduo::Logger::DEBUG, __func__).stream()
#define LOG_INFO                                              \
    if (mymuduo::Logger::logLevel() <= mymuduo::Logger::INFO) \
    mymuduo::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN mymuduo::Logger(__FILE__, __LINE__, mymuduo::Logger::WARN).stream()
#define LOG_ERROR mymuduo::Logger(__FILE__, __LINE__, mymuduo::Logger::ERROR).stream()
#define LOG_FATAL mymuduo::Logger(__FILE__, __LINE__, mymuduo::Logger::FATAL).stream()

} // namespace mymuduo