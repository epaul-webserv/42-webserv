#ifndef IRESPONSEBODY_HPP
#define IRESPONSEBODY_HPP

#include <async>

class IResponseBody
{
  public:
    virtual async::Future::Poll poll_write(container::Slice<uint8_t> buf,
                                           size_t                   &s) throw(async::OsError) = 0;
    virtual async::Future::Poll poll_end() throw(async::OsError)            = 0;
    virtual ~IResponseBody() {}
};

#endif
