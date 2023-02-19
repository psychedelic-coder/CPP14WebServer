#include "mymuduo/FileUtil.h"

#include <stdio.h>
namespace mymuduo
{
    /**
     * @brief 底层调用-->FILE *fopen(const char *__restrict__ __filename, const char *__restrict__ __modes)
     * The fopen() function opens the file whose name is the string pointed to by pathname and associates a stream with
       it.
     * @param char* modes -->  'a' │ O_WRONLY | O_CREAT | O_APPEND 'e' for O_CLOEXEC 
     */
    AppendFile::AppendFile(std::string filename)
        : fp_(fopen(filename.c_str(), "ae")) // 'e' for O_CLOEXEC
    {
        // 用户提供缓冲区，将fp_缓冲区设置为本地的buffer_
        ::setbuffer(fp_, buffer_, sizeof buffer_);
    }

    AppendFile::~AppendFile()
    {
        ::fclose(fp_);
    }
    // append 向文件写
    void AppendFile::append(const char *logline, const size_t len)
    {
        // 已经写到文件的长度
        size_t n = this->write(logline, len);
        // 还剩多少没有写到文件
        size_t remain = len - n;
        while (remain > 0)
        {
            size_t x = this->write(logline + n, remain);
            if (x == 0)
            {
                int err = ferror(fp_);
                if (err)
                    fprintf(stderr, "AppendFile::append() failed !\n");
                break;
            }
            n += x;
            remain = len - n;
        }
    }

    void AppendFile::flush()
    {
        /**
         * -----int fflush(FILE *stream);
         * @brief 对于输出文件流，将缓冲区所有数据写到给定输出或通过文件流的underlying写函数更新文件流。场景是写数据到stream
         *      For  output  streams,  fflush()  forces  a  write of all user-space buffered data
         *      for the given output or update stream via the stream's underlying write function.
         *        对于关联到seekable文件(比如磁盘)的输入文件流，fflush丢弃掉缓冲区中还没来得及被应用消费的数据，也就是从underlying
         *      读取文件取数据到了缓冲区，缓冲区的数据被丢弃不会被利用。场景是从seekable文件读数据
         *      For input streams associated with seekable files (e.g., disk files, but not pipes or terminals),  
         *      fflush()  discards  any buffered data that has been fetched from the underlying file, 
         *      but has not been consumed by the application.
         * @
         */
        ::fflush(fp_);
    }

    size_t AppendFile::write(const char *logline, size_t len)
    {
        /**
         * size_t fwrite(const void* buffer, size_t size, size_t count, FILE* stream);
         * -- buffer:指向数据块的指针
         * -- size:每个数据的大小，单位为Byte(例如：sizeof(int)就是4)
         * -- count:数据个数
         * -- stream:文件指针
         * @return  On  success,  fread() and fwrite() return the number of items read or written.
         */
        return ::fwrite_unlocked(logline, 1, len, fp_);
    }
} // namespace mymuduo
