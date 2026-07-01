#include "ChunkedResponseBody.hpp"
#include <sstream>

ChunkedResponseBody::ChunkedResponseBody(async::BufIo<65535> &io)
    : io(io), _chunk_written(0), _chunk_filled(0),
      _last_chunk(const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>("0\r\n\r\n")), 5), _buf(),
      _reserved(16), _header_size(0)
{
    _buf.resize(65535);
    std::ostringstream oss;
    oss << std::hex << _buf.size() - _reserved << "\r\n";
    _header_size = oss.str().size();
    std::copy(oss.str().c_str(), oss.str().c_str() + _header_size, &_buf[0]);
}

ChunkedResponseBody::~ChunkedResponseBody() {}

async::Future::Poll ChunkedResponseBody::poll_write(container::Slice<uint8_t> buf,
                                                    size_t &s) throw(async::OsError)
{
    if (_chunk_filled < _buf.size() - _reserved)
    {
        size_t available_space = (_buf.size() - _reserved) - _chunk_filled;
        s                      = std::min(buf.len(), available_space);
        buf                    = buf.slice(0, s);
        std::copy(buf.begin(), buf.end(), &_buf[_header_size + _chunk_filled]);
        _chunk_filled += s;
        return (async::Future::Ready);
    }
    size_t written_bytes;
    _buf[_header_size + _chunk_filled]     = '\r';
    _buf[_header_size + _chunk_filled + 1] = '\n';
    if (io.poll_write(container::Slice<uint8_t>(&_buf[0], _chunk_filled + _header_size + 2),
                      written_bytes) == async::Future::Pending)
        return (async::Future::Pending);
    _chunk_written += written_bytes - _header_size - 2;
    if (_chunk_written == _chunk_filled)
    {
        _chunk_written = 0;
        _chunk_filled  = 0;
    }
    s = 0;
    return (async::Future::Ready);
}

#include <iostream>
async::Future::Poll ChunkedResponseBody::poll_end() throw(async::OsError)
{
    size_t s = 0;

    while (_chunk_written < _chunk_filled)
    {
        std::ostringstream oss;
        size_t             to_deduct = 0;

        oss << std::hex << _chunk_filled - _chunk_written << "\r\n";
        to_deduct += oss.str().size();
        oss.write(
            reinterpret_cast<const std::ostream::char_type *>(&_buf[_header_size + _chunk_filled]),
            _chunk_filled - _chunk_written);
        oss << "\r\n";
        to_deduct += 2;
        if (io.poll_write(container::Slice<uint8_t>(
                              reinterpret_cast<unsigned char *>(&oss.str()[0]), oss.str().size()),
                          s) == async::Future::Pending)
            return (async::Future::Pending);
        _chunk_written += s - to_deduct;
        if (_chunk_written == _chunk_filled)
        {
            _chunk_filled  = 0;
            _chunk_written = 0;
            break;
        }
    }
    if (_last_chunk.len() > 0 && io.poll_write(_last_chunk, s) == async::Future::Pending)
        return (async::Future::Pending);
    _last_chunk = _last_chunk.slice(s, _last_chunk.len());
    if (_last_chunk.len() == 0 && io.poll_flush() == async::Future::Pending)
        return (async::Future::Pending);
    return (async::Future::Ready);
}
