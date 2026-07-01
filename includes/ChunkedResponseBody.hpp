#ifndef CHUNKEDRESPONSEBODY_HPP
#define CHUNKEDRESPONSEBODY_HPP
#include "IResponseBody.hpp"

class ChunkedResponseBody : public IResponseBody
{
  private:
    async::BufIo<65535>      &io;
    size_t                    _chunk_written;
    size_t                    _chunk_filled;
    size_t                    _buf_filled;
    size_t                    _buf_written;
    container::Slice<uint8_t> _last_chunk;
    std::vector<uint8_t>      _buf;
    unsigned int              _reserved;
    unsigned int              _header_size;
    bool                      _last_write;

  public:
    ChunkedResponseBody(async::BufIo<65535> &io);
    ~ChunkedResponseBody();
    async::Future::Poll poll_write(container::Slice<uint8_t> buf, size_t &s) throw(async::OsError);
    async::Future::Poll poll_end() throw(async::OsError);
};

#endif
