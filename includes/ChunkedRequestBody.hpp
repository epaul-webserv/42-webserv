#ifndef CHUNKEDREQUESTBODY_HPP
#define CHUNKEDREQUESTBODY_HPP

#include "IRequestBody.hpp"

class ChunkedRequestBody : public IRequestBody
{
private:
	async::BufIo<>			 &io;
	uint8_t					  _buf[4096];
	container::Slice<uint8_t> _sl;
	bool					  _fail;
	bool					  _end;
	size_t					  _chunk_read;
	size_t					  _chunk_to_read;
	size_t					  _buf_read;
	size_t					  _buf_write;

public:
	ChunkedRequestBody(async::BufIo<> &io);
	~ChunkedRequestBody();
	bool				fail();
	async::Future::Poll poll_read(container::Slice<uint8_t> &buf, size_t &call_written) throw(async::OsError);
};

#endif
