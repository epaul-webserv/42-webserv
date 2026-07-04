#include "ChunkedResponseBody.hpp"
#include <sstream>

ChunkedResponseBody::ChunkedResponseBody(async::BufIo<65535> &io)
    : io(io), _chunk_written(0), _chunk_filled(0), _buf_filled(0), _buf_written(0),
      _last_chunk(const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>("0\r\n\r\n")), 5), _buf(),
      _reserved(16), _header_size(0), _last_write(false)
{
    _buf.resize(65535);
    std::ostringstream oss;
    oss << std::hex << _buf.size() - _reserved << "\r\n";
    _header_size    = oss.str().size();
    _buf_filled     = _header_size;
    std::string str = oss.str();
    std::copy(str.begin(), str.end(), &_buf[0]);
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
        _buf_filled += s;
        return (async::Future::Ready);
    }
    size_t written_bytes;
    _buf[_buf_filled]     = '\r';
    _buf[_buf_filled + 1] = '\n';
    while (_buf_written < _buf_filled + 2)
    {
        if (io.poll_write(
                container::Slice<uint8_t>(&_buf[_buf_written], _buf_filled + 2 - _buf_written),
                written_bytes) == async::Future::Pending)
            return (async::Future::Pending);
        _buf_written += written_bytes;
        if (_buf_written > _header_size)
            _chunk_written = std::min(_buf_written - _header_size, _buf_filled - _header_size);
    }
    if (_buf_written == _buf_filled + 2)
    {
        _chunk_written = 0;
        _chunk_filled  = 0;
        _buf_written   = 0;
        _buf_filled    = _header_size;
    }
    s = 0;
    return (async::Future::Ready);
}

#include <iostream>

async::Future::Poll ChunkedResponseBody::poll_end() throw(async::OsError)
{
    size_t s = 0;

    if (_chunk_filled - _chunk_written > 0)
    {
        if (!_last_write)
        {
            std::ostringstream oss;
            oss << std::hex << _chunk_filled - _chunk_written << "\r\n";
            oss.write(reinterpret_cast<const std::ostream::char_type *>(
                          &_buf[_header_size + _chunk_written]),
                      _chunk_filled - _chunk_written);
            oss << "\r\n";
            std::string str = oss.str();
            std::copy(str.begin(), str.end(), &_buf[0]);
            _buf_filled  = str.size();
            _buf_written = 0;
            _last_write  = true;
        }
        while (_buf_written < _buf_filled)
        {
            if (io.poll_write(
                    container::Slice<uint8_t>(&_buf[_buf_written], _buf_filled - _buf_written),
                    s) == async::Future::Pending)
                return (async::Future::Pending);
            _buf_written += s;
            if (_buf_written == _buf_filled)
            {
                _buf_filled  = 0;
                _buf_written = 0;
                break;
            }
        }
    }
    if (_last_chunk.len() > 0 && io.poll_write(_last_chunk, s) == async::Future::Pending)
        return (async::Future::Pending);
    _last_chunk = _last_chunk.slice(s, _last_chunk.len());
    if (_last_chunk.len() == 0 && io.poll_flush() == async::Future::Pending)
        return (async::Future::Pending);
    return (async::Future::Ready);
}
