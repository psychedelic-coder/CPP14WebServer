#pragma once

#include <iostream>
#include <string>

namespace mymuduo
{
    class Timestamp
    {
    public:
        ///
        /// Constucts an invalid Timestamp.
        ///
        Timestamp()
            : microSecondsSinceEpoch_(0)
        {
        }
        // Timestamp();
        explicit Timestamp(int64_t microSecondsSinceEpoch_);
        static Timestamp now();
        /// @brief  return an invalid Timestamp.
        /// @return an invalid Timestamp.
        static Timestamp invalid()
        {
            return Timestamp();
        }
        std::string tostring() const;

        bool valid() const { return microSecondsSinceEpoch_ > 0; }

        static const int kMicroSecondsPerSecond = 1000 * 1000;
        // for internal usage.
        int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }

    private:
        int64_t microSecondsSinceEpoch_;
    };

    inline Timestamp addTime(Timestamp timestamp, double seconds)
    {
        int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
        return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
    }

    inline bool operator<(Timestamp lhs, Timestamp rhs)
    {
        return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
    }

    inline bool operator==(Timestamp lhs, Timestamp rhs)
    {
        return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
    }
} // namespace mymuduo