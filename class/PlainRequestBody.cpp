#include "PlainRequestBody.hpp"

PlainRequestBody::PlainRequestBody(async::BufIo<65535> &io, size_t const &to_read)
    : _fail(false), io(io), _to_read(to_read), _read(0)
{
}

PlainRequestBody::~PlainRequestBody() {}

bool PlainRequestBody::fail() { return (_fail); };

async::Future::Poll PlainRequestBody::poll_read(container::Slice<uint8_t> &buf, size_t &s)
{
    if (_fail || (_to_read == _read))
    {
        s = 0;
        return (async::Future::Ready);
    }
    size_t                    amt = std::min(buf.len(), _to_read - _read);
    container::Slice<uint8_t> tmp = buf.slice(0, amt);
    if (io.poll_read(tmp, s) == async::Future::Pending)
        return (async::Future::Pending);
    if (!s && _read != _to_read)
    {
        _fail = true;
        throw(BadReadException());
    }
    _read += s;
    return async::Future::Ready;
}
