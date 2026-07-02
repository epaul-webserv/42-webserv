#ifndef CHUNKEDREQUESTBODY_HPP
#define CHUNKEDREQUESTBODY_HPP

#include "IRequestBody.hpp"
#include <vector>

class ChunkedRequestBody : public IRequestBody
{
  private:
    async::BufIo<65535>      &io;
    uint8_t                   _buf[65535];
    container::Slice<uint8_t> _sl;
    bool                      _fail;
    bool                      _end;
    size_t                    _chunk_read;
    size_t                    _chunk_to_read;
    size_t                    _buf_read;
    size_t                    _buf_write;
    ssize_t                   _max_size;
    size_t                    _content_length;
    std::vector<uint8_t>      _body;
    std::vector<uint8_t>      _trailer;
    std::string              &_header;
    std::string               _reminder;
    bool                      _read_trailers;

  public:
    ChunkedRequestBody(async::BufIo<65535> &io, ssize_t max_size,
                       std::string &content_length_header);
    ~ChunkedRequestBody();
    bool                fail();
    async::Future::Poll poll_read(container::Slice<uint8_t> &buf, size_t &call_written);
    async::Future::Poll fill_buf();
    bool                header_cropped();
    async::Future::Poll consume_trailers();
};

#endif
