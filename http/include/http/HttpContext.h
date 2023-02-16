#pragma once

#include "http/HttpRequest.h"

namespace mymuduo
{
    class Buffer;
} // namespace mymuduo

namespace http
{
    class HttpContext
    {
    public:
        enum HttpRequestParseState
        {
            kExpectRequestLine,
            kExpectHeaders,
            kExpectBody,
            kGotAll,
        };

        HttpContext()
            : state_(kExpectRequestLine)
        {
        }

        // default copy-ctor, dtor and assignment are fine

        // return false if any error
        bool parseRequest(mymuduo::Buffer *buf, mymuduo::Timestamp receiveTime);

        bool gotAll() const
        {
            return state_ == kGotAll;
        }

        void reset()
        {
            state_ = kExpectRequestLine;
            HttpRequest dummy;
            request_.swap(dummy);
        }

        const HttpRequest &request() const
        {
            return request_;
        }

        HttpRequest &request()
        {
            return request_;
        }

    private:
        bool processRequestLine(const char *begin, const char *end);

        HttpRequestParseState state_;
        HttpRequest request_;
    };
} // namespace http
