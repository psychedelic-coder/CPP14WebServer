#include "mymuduo/Timestamp.h"

#include <time.h>

namespace mymuduo
{
    // Timestamp::Timestamp() : microSecondsSinceEpoch_(0) {}

    Timestamp::Timestamp(int64_t microSecondsSinceEpoch_)
        : microSecondsSinceEpoch_(microSecondsSinceEpoch_)
    {
    }

    Timestamp Timestamp::now()
    {
        return Timestamp(time(NULL));
    }

    std::string Timestamp::tostring() const
    {
        char buf[128] = {0};
        tm *tm_time = localtime(&microSecondsSinceEpoch_);
        snprintf(buf, 128, "%4d/%02d/%02d   %02d:%02d:%02d",
                 tm_time->tm_year + 1900,
                 tm_time->tm_mon + 1,
                 tm_time->tm_mday,
                 tm_time->tm_hour,
                 tm_time->tm_min,
                 tm_time->tm_sec);

        return buf;
    }
}

// #include <iostream>

// int main()
//{
//     std::cout << mymuduo::Timestamp::now().tostring() << std::endl;
//     return 0;
// }