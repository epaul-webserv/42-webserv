#include "PlainResponseBody.hpp"

PlainResponseBody::PlainResponseBody(async::BufIo<65535> &io) : io(io) {}

async::Future::Poll PlainResponseBody::poll_write(container::Slice<uint8_t> buf,
                                                  size_t                   &s) throw(async::OsError)
{
    return io.poll_write(buf, s);
}

async::Future::Poll PlainResponseBody::poll_end() throw(async::OsError) { return io.poll_flush(); }
