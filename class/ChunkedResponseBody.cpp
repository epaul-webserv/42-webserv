#include "ChunkedResponseBody.hpp"
#include <sstream>

ChunkedResponseBody::ChunkedResponseBody(async::BufIo<> &io)
    : io(io), _chunk_written(0), _chunk_filled(0),
      _last_chunk(const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>("0\r\n\r\n")), 5),
      _buf_size(4096), _buf(_buf_size)
{
    _buf[0]    = 'F';
    _buf[1]    = 'F';
    _buf[2]    = '9';
    _buf[3]    = '\r';
    _buf[4]    = '\n';
    _buf[4094] = '\r';
    _buf[4095] = '\n';
}

ChunkedResponseBody::~ChunkedResponseBody() {}

async::Future::Poll ChunkedResponseBody::poll_write(container::Slice<uint8_t> buf,
                                                    size_t &s) throw(async::OsError)
{
    if (_chunk_filled < _buf_size - 7)
    {
        s   = std::min(buf.len(), _buf_size - 7 - _chunk_filled);
        buf = buf.slice(0, s);
        _buf.insert(_buf.begin() + 5 + _chunk_filled, buf.begin(), buf.end());
        _chunk_filled += s;
    }
    if (_chunk_filled < _buf_size - 7)
        return (async::Future::Ready);
    size_t written_bytes;
    _buf[4094] = '\r';
    _buf[4095] = '\n';
    if (io.poll_write(container::Slice<uint8_t>(&_buf[0], _buf_size), written_bytes) ==
        async::Future::Pending)
        return (async::Future::Pending);
    _chunk_written += written_bytes;
    if (_chunk_written == _buf_size)
    {
        _chunk_written = 0;
        _chunk_filled  = 0;
    }
    s = 0;
    return (async::Future::Ready);
}

async::Future::Poll ChunkedResponseBody::poll_end() throw(async::OsError)
{
    size_t s = 0;

    while (_chunk_written < _chunk_filled)
    {
        std::ostringstream oss;
        size_t             to_deduct = 0;

        oss << std::hex << _chunk_filled - _chunk_written << "\r\n";
        to_deduct += oss.str().size();
        oss.write(reinterpret_cast<const std::ostream::char_type *>(&_buf[5]),
                  _chunk_filled - _chunk_written);
        oss << "\r\n";
        to_deduct += 2;
        if (io.poll_write(container::Slice<uint8_t>(
                              reinterpret_cast<unsigned char *>(&oss.str()[0]), oss.str().size()),
                          s) == async::Future::Pending)
            return (async::Future::Pending);
        _chunk_written += s;
    }
    if (_last_chunk.len() > 0 && io.poll_write(_last_chunk, s) == async::Future::Pending)
        return (async::Future::Pending);
    _last_chunk = _last_chunk.slice(s, _last_chunk.len());
    if (_last_chunk.len() == 0 && io.poll_flush() == async::Future::Pending)
        return (async::Future::Pending);
    return (async::Future::Ready);
}
