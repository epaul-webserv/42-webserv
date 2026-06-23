#include "ChunkedResponseBody.hpp"
#include <sstream>

ChunkedResponseBody::ChunkedResponseBody(async::BufIo<> &io) : io(io), _last_chunk(const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>("0\r\n\r\n")), 5) {}

ChunkedResponseBody::~ChunkedResponseBody() {}

async::Future::Poll ChunkedResponseBody::poll_write(container::Slice<uint8_t> buf, size_t &s) throw(async::OsError)
{
	size_t			   to_deduct = 0;
	std::ostringstream oss;
	size_t			   buf_size = std::min(buf.len(), static_cast<size_t>(4000));
	oss << std::hex << buf_size << "\r\n";
	to_deduct += oss.str().size();
	if (buf.len() > 0)
		oss.write(reinterpret_cast<const std::ostream::char_type *>(&buf[0]), buf_size);
	oss << "\r\n";
	to_deduct += 2;
	if (io.poll_write(container::Slice<uint8_t>(reinterpret_cast<unsigned char *>(&oss.str()[0]), oss.str().size()), s) == async::Future::Pending)
		return async::Future::Pending;
	s -= to_deduct;
	return async::Future::Ready;
}

async::Future::Poll ChunkedResponseBody::poll_end() throw(async::OsError)
{
	size_t s = 0;
	if (_last_chunk.len() > 0 && io.poll_write(_last_chunk, s) == async::Future::Pending)
		return (async::Future::Pending);
	_last_chunk = _last_chunk.slice(s, _last_chunk.len());
	if (_last_chunk.len() == 0 && io.poll_flush_write() == async::Future::Pending)
		return (async::Future::Pending);
	return (async::Future::Ready);
}
