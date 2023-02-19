#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace mymuduo
{
    namespace CurrentThread
    {
        // internal     thread_local
        extern __thread int t_cachedTid;

        void cacheTid();

        inline int tid()
        {
            // 如果t_cachedTid == 0，说明还没::syscall(SYS_gettid);
            if (__builtin_expect(t_cachedTid == 0, 0)) // __builtin_expect作用是允许程序员将最有可能执行的分支告诉编译器进行jmp优化
            {
                cacheTid();
            }

            return t_cachedTid;
        }

    } // namespace CurrentThread

} // namespace mymuduo
