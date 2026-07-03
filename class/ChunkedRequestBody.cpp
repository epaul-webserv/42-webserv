#include "ChunkedRequestBody.hpp"
#include <sstream>

ChunkedRequestBody::ChunkedRequestBody(async::BufIo<65535> &io, ssize_t max_size,
                                       std::string &content_length_header)
    : io(io), _buf(), _sl(_buf, sizeof(_buf) - 1), _fail(false), _end(false), _chunk_read(0),
      _chunk_to_read(0), _buf_read(0), _buf_write(0), _max_size(max_size), _content_length(0),
      _header(content_length_header), _read_trailers(false)
{
}

ChunkedRequestBody::~ChunkedRequestBody() {}

bool ChunkedRequestBody::fail() { return (_fail); }

bool ChunkedRequestBody::header_cropped()
{
    size_t i;
    for (i = _buf_read; _buf[i] && (std::isxdigit(_buf[i]) || _buf[i] == '\r'); ++i)
        ;
    if (!_buf[i])
    {
        _reminder.insert(_reminder.end(), _buf + _buf_read, _buf + i);
        return (true);
    }
    return (false);
}

async::Future::Poll ChunkedRequestBody::consume_trailers()
{
    if (_trailer.size() == 1 && _trailer[0] == '\n')
    {
        _end = true;
        return (async::Future::Ready);
    }
    if (_trailer.size() == 2 && _trailer[0] == '\r' && _trailer[1] == '\n')
    {
        _end = true;
        return (async::Future::Ready);
    }
    // if (_trailer.empty() || _trailer[_trailer.size() - 1] != '\n')
    //     throw(BadReadException());
    return (async::Future::Pending);
}

async::Future::Poll ChunkedRequestBody::fill_buf()
{
    if (_end)
        return (async::Future::Ready);
    for (;;)
    {
        if (_read_trailers)
        {
            if (io.poll_read_line(_trailer) == async::Future::Pending)
                return (async::Future::Pending);
        }
        else
        {
            if (_chunk_to_read == 0)
                _sl.set_len(1);
            else
                _sl.set_len(_chunk_to_read - _chunk_read);
            if (io.poll_read(_sl, _buf_write) == async::Future::Pending)
                return (async::Future::Pending);
            _sl[_buf_write] = '\0';
            _buf_read       = 0;
            if (_buf_write == 0)
            {
                if (_chunk_read != _chunk_to_read)
                {
                    _fail = true;
                    throw(BadReadException());
                }
                break;
            }
        }
        while (_buf_read < _buf_write)
        {
            if (_read_trailers)
            {
                if (consume_trailers() == async::Future::Ready)
                    return (async::Future::Ready);
                break;
            }
            if (_chunk_read >= _chunk_to_read)
            {
                if (_reminder.empty() && _sl[_buf_read] == '\r')
                    ++_buf_read;
                if (_buf_read == _buf_write)
                    continue;
                if (_reminder.empty() && _sl[_buf_read] == '\n')
                    ++_buf_read;
                if (_buf_read == _buf_write)
                    continue;
                _chunk_read = 0;
                if (header_cropped())
                {
                    _chunk_to_read = 0;
                    break;
                }
                char       *end;
                std::string joined(_reminder + reinterpret_cast<const char *>(&_sl[_buf_read]));
                _chunk_to_read = std::strtoul(joined.c_str(), &end, 16);
                _buf_read += reinterpret_cast<uint8_t *>(end) -
                             reinterpret_cast<const unsigned char *>(joined.c_str()) -
                             _reminder.size();
                if (end[0] == '\r' && end[1] == '\n')
                    _buf_read += 2;
                else if (end[0] == '\n')
                    _buf_read += 1;
                else
                {
                    _fail = true;
                    throw(BadReadException());
                }
                _reminder.clear();
                if (_chunk_to_read == 0)
                {
                    _buf_read      = 0;
                    _read_trailers = true;
                    // _end = true;
                    _body.insert(_body.end(), '\0');
                    std::ostringstream oss;
                    oss << _content_length;
                    _header = oss.str();
                    break;
                    // return (async::Future::Ready);
                }
            }
            if (_chunk_read < _chunk_to_read)
            {
                size_t to_read = std::min(_buf_write - _buf_read, _chunk_to_read - _chunk_read);
                _body.insert(_body.end(), _buf + _buf_read, _buf + _buf_read + to_read);
                _chunk_read += to_read;
                _content_length += to_read;
                _buf_read += to_read;
                if (_max_size >= 0 && _content_length > static_cast<size_t>(_max_size))
                {
                    _fail = true;
                    throw(TooLargeException());
                }
                if (_chunk_read >= _chunk_to_read)
                    _chunk_to_read = 0;
            }
        }
    }
    _buf_read = 0;
    _body.insert(_body.end(), 0);
    return (async::Future::Ready);
}

async::Future::Poll ChunkedRequestBody::poll_read(container::Slice<uint8_t> &buf,
                                                  size_t                    &call_written)
{
    if (_fail || (_end && _content_length == _buf_read))
        return (call_written = 0, async::Future::Ready);
    if (!_end && fill_buf() == async::Future::Pending)
        return (async::Future::Pending);
    size_t amt = std::min(buf.len(), _content_length - _buf_read);
    std::copy(&_body[_buf_read], &_body[_buf_read + amt], buf.begin());
    _buf_read += amt;
    call_written = amt;
    return (async::Future::Ready);
}
