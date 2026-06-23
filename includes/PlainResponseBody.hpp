#ifndef PLAINRESPONSEBODY_HPP
#define PLAINRESPONSEBODY_HPP

#include "IResponseBody.hpp"

class PlainResponseBody : public IResponseBody
{
private:
	async::BufIo<> &io;

public:
	PlainResponseBody(async::BufIo<> &io);
	async::Future::Poll poll_write(container::Slice<uint8_t> buf, size_t &s) throw(async::OsError);
	async::Future::Poll poll_end() throw(async::OsError);
};

#endif
