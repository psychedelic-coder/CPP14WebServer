#include "mymuduo/LogFile.h"

#include <string>
namespace mymuduo
{
    LogFile::LogFile(const std::string &basename, int flushEveryN)
        : basename_(basename),
          flushEveryN_(flushEveryN),
          count_(0),
          mutex_(new std::mutex)
    {
        // assert(basename.find('/') >= 0);
        file_.reset(new AppendFile(basename));
    }

    LogFile::~LogFile() {}

    void LogFile::append(const char *Logline, int len)
    {
        // 从lock_guard<>在自身作用域（生命周期）中具有构造时加锁，析构时解锁的功能。
        std::lock_guard<std::mutex> lock(*mutex_);
        appendInLock(Logline, len);
    }

    void LogFile::flush()
    {
        std::lock_guard<std::mutex> lock(*mutex_);
        file_->flush();
    }

    void LogFile::appendInLock(const char *Logline, int len)
    {
        file_->append(Logline, len);
        ++count_;
        if (count_ >= flushEveryN_)
        {
            count_ = 0;
            file_->flush();
        }
    }

} // namespace mymuduo
