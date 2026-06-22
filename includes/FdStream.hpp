#ifndef FDSTREAM_HPP
#define FDSTREAM_HPP

#include "stdint.h"
#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <string>
#include <unistd.h>
#include <vector>

#define FDSTREAM_BUFF_SIZE 8192

class FdStream
{
  public:
    FdStream(int fd);
    ~FdStream();

    FdStream            &operator=(FdStream const &other);
    bool                 end() const;
    std::string          get_str(size_t n);
    void                 consume(size_t n);
    std::vector<uint8_t> peek_str(size_t n);
    std::string          get_str_until(char c);
    bool                 is_open() const;
    void                 clear(int fd);

    class InvalidFdException : public std::exception
    {
        const char *what() const throw();
    };

    class InvalidReadException : public std::exception
    {
        const char *what() const throw();
    };

  private:
    FdStream(FdStream const &FdStream);
    void fillBuffer();

    bool   _end;
    int    _fd;
    char   _buffer[FDSTREAM_BUFF_SIZE];
    size_t _read_index;
    size_t _write_index;
};

#endif
