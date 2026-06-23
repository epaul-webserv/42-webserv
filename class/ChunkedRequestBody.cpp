#include "ChunkedRequestBody.hpp"

ChunkedRequestBody::ChunkedRequestBody(async::BufIo<> &io) :
		io(io), _buf(), _sl(_buf, sizeof(_buf)), _fail(false), _end(false), _chunk_read(0), _chunk_to_read(0), _buf_read(0), _buf_write(0)
{
}

ChunkedRequestBody::~ChunkedRequestBody() {}

bool				ChunkedRequestBody::fail() { return (_fail); };

async::Future::Poll ChunkedRequestBody::poll_read(container::Slice<uint8_t> &buf, size_t &call_written) throw(async::OsError)
{
	if (_fail || _end)
		return (call_written = 0, async::Future::Ready);
	for (call_written = 0; call_written < buf.len();)
	{
		if (_buf_write - _buf_read <= 0)
		{
			if (io.poll_read(_sl, _buf_write) == async::Future::Pending)
				return (async::Future::Pending);
			if (_buf_write == 0)
			{
				if (_chunk_read != _chunk_to_read)
					_fail = true;
				break;
			}
		}
		if (_buf_write - _buf_read > 0)
		{
			if (_chunk_read < _chunk_to_read)
			{
				size_t amt = std::min(std::min(_sl.len() - _buf_read, buf.len() - call_written), _chunk_to_read - _chunk_read);
				std::copy(&_sl[_buf_read], &_sl[_buf_read + amt], buf.begin() + call_written);
				_buf_read += amt;
				_chunk_read += amt;
				call_written += amt;
			}
			else
			{
				if (_sl[_buf_read] == '\r' && _sl[_buf_read + 1] == '\n')
					_buf_read += 2;
				char *end;
				_chunk_to_read = std::strtoul(reinterpret_cast<const char *>(&_sl[_buf_read]), &end, 16);
				_chunk_read	   = 0;
				_buf_read += reinterpret_cast<uint8_t *>(end) - (_buf + _buf_read);
				if (end[0] == '\r' && end[1] == '\n')
					_buf_read += 2;
				else
				{
					_fail = true;
					return (async::Future::Ready);
				}
				if (_chunk_to_read == 0)
				{
					_end = true;
					break;
				}
			}
		}
	}
	if (io.poll_read(_sl, _buf_write) == async::Future::Pending)
		return (async::Future::Ready); // FIXME: don't remember why I did this
	return (async::Future::Ready);
}
