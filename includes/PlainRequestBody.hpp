#ifndef PLAINREQUESTBODY_HPP
#define PLAINREQUESTBODY_HPP

#include "IRequestBody.hpp"

class PlainRequestBody : public IRequestBody
{
  private:
    bool            _fail;
    async::BufIo<> &io;
    size_t          _to_read;
    size_t          _read;

  public:
    PlainRequestBody(async::BufIo<> &io, size_t const &to_read);
    ~PlainRequestBody();
    bool                fail();
    async::Future::Poll poll_read(container::Slice<uint8_t> &buf, size_t &s) throw(async::OsError);
};

#endif
