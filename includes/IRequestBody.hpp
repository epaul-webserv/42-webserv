#ifndef IREQUESTBODY_HPP
#define IREQUESTBODY_HPP

#include <async>

class IRequestBody
{
  public:
    virtual async::Future::Poll poll_read(container::Slice<uint8_t> &buf,
                                          size_t                    &s) throw(async::OsError) = 0;
    virtual bool                fail()                                     = 0;
    virtual ~IRequestBody() {}
};

#endif
