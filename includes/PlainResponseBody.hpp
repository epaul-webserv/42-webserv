#ifndef PLAINRESPONSEBODY_HPP
#define PLAINRESPONSEBODY_HPP

#include "IResponseBody.hpp"

class PlainResponseBody : public IResponseBody
{
  private:
    async::BufIo<65535> &io;

  public:
    PlainResponseBody(async::BufIo<65535> &io);
    async::Future::Poll poll_write(container::Slice<uint8_t> buf, size_t &s) throw(async::OsError);
    async::Future::Poll poll_end() throw(async::OsError);
};

#endif
