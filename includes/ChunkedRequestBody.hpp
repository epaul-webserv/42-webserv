#ifndef CHUNKEDREQUESTBODY_HPP
#define CHUNKEDREQUESTBODY_HPP

#include <algorithm>
#include <async>
#include <cstddef>
#include <stdexcept>
#include <stdint.h>

#include "IRequestBody.hpp"
#include "future.hpp"

template <size_t N> class ChunkedRequestBody : public IRequestBody
{
  private:
    async::BufIo<N> &io;
    // -1: no max size
    ssize_t max_size;
    size_t  read_total;
    ssize_t current_chunk_size;
    enum
    {
        CHUNK_SIZE,
        CHUNK_SIZE_CRLF,
        CHUNK_DATA,
        CHUNK_DATA_CR,
        CHUNK_DATA_LF,
    } step;
    size_t chunk_size;
    size_t chunk_read;

  public:
    ChunkedRequestBody(async::BufIo<N> &io, ssize_t max_size)
        : io(io), max_size(max_size), read_total(0), current_chunk_size(0), step(CHUNK_SIZE),
          chunk_size(0), chunk_read(0)
    {
    }
    ~ChunkedRequestBody() {}

    static int from_hex(uint8_t c)
    {
        if ('0' <= c && c <= '9')
            return (c - '0');
        if ('A' <= c && c <= 'F')
            return (c - 'A');
        if ('a' <= c && c <= 'f')
            return (c - 'a');
        return -1;
    }

    async::Future::Poll poll_read(container::Slice<uint8_t> &buf, size_t &s)
    {
        container::Slice<uint8_t>           b;
        container::Slice<uint8_t>::iterator it;

        for (;;)
        {

            if (io.poll_fill_buf(b) == async::Future::Pending)
                return async::Future::Pending;
            switch (step)
            {
            case CHUNK_SIZE:
                if (b.len() == 0)
                    throw std::runtime_error("unexpected EOF");
                for (it = b.begin(); it != b.end(); it++)
                {
                    int x = from_hex(*it);
                    if (x < 0)
                    {
                        io.consume(it - b.begin());
                        step = CHUNK_SIZE_CRLF;
                        current_chunk_size += chunk_size;
                        if (max_size >= 0 && current_chunk_size < max_size)
                            throw TooLargeException();
                        break;
                    }
                    chunk_size = chunk_size * 16 + x;
                }
                if (it == b.end())
                    io.consume(b.len());
                break;
            case CHUNK_SIZE_CRLF:
                if (b.len() == 0)
                    throw std::runtime_error("unexpected EOF");
                for (it = b.begin(); it != b.end(); it++)
                {
                    // FIXME: we accept a bit more the `chunk-ext`
                    if (*it == '\n')
                    {
                        io.consume(it - b.begin() + 1);
                        if (chunk_size == 0)
                        {
                            // FIXME: trailers
                            s = 0;
                            return async::Future::Ready;
                        }
                        step       = CHUNK_DATA;
                        chunk_read = 0;
                        break;
                    }
                }
                if (it == b.end())
                    io.consume(b.len());
                break;
            case CHUNK_DATA:
                if (b.len() == 0)
                    throw std::runtime_error("unexpected EOF");
                s = b.len();
                if (s >= chunk_size - chunk_read)
                    s = chunk_size - chunk_read;
                s = std::min(s, buf.len());
                std::copy(b.begin(), b.begin() + s, buf.begin());
                chunk_read += s;
                if (chunk_size == chunk_read)
                    step = CHUNK_DATA_CR;
                io.consume(s);
                return async::Future::Ready;
            case CHUNK_DATA_CR:
                if (b.len() == 0)
                    throw std::runtime_error("unexpected EOF");
                switch (b[0])
                {
                case '\r':
                    step = CHUNK_DATA_LF;
                    break;
                case '\n':
                    step       = CHUNK_SIZE;
                    chunk_size = 0;
                    break;
                default:
                    throw std::runtime_error("unexpected bytes");
                }
                io.consume(1);
                break;
            case CHUNK_DATA_LF:
                if (b.len() == 0)
                    throw std::runtime_error("unexpected EOF");
                if (b[0] != '\n')
                    throw std::runtime_error("unexpected bytes");
                step       = CHUNK_SIZE;
                chunk_size = 0;
                io.consume(1);
                break;
            }
        }
    }
};

#endif
