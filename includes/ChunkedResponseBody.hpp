#ifndef CHUNKEDRESPONSEBODY_HPP
#define CHUNKEDRESPONSEBODY_HPP

#include "IResponseBody.hpp"

class ChunkedResponseBody : public IResponseBody
{
  private:
    async::BufIo<>           &io;
    size_t                    _chunk_written;
    size_t                    _chunk_filled;
    container::Slice<uint8_t> _last_chunk;
    size_t const              _buf_size;
    std::vector<uint8_t>      _buf;

  public:
    ChunkedResponseBody(async::BufIo<> &io);
    ~ChunkedResponseBody();
    async::Future::Poll poll_write(container::Slice<uint8_t> buf, size_t &s) throw(async::OsError);
    async::Future::Poll poll_end() throw(async::OsError);
};

#endif
