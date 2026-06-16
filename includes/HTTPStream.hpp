#ifndef WEBSERV_HTTPSTREAM_HPP
#define WEBSERV_HTTPSTREAM_HPP

#include <async>
#include <iostream>
#include <limits>
#include <sys/socket.h>
#include "Request.hpp"
#include "Response.hpp"
#include "Router.hpp"

class Router;

enum stream_status
{
	_fetching_request,
	_getting_backend,
	_processing_response,
	_sending_response
};

enum headers_status
{
	_initializing_headers,
	_sending_headers,
	_headers_sent
};


class ARequestBody
{
public:
	virtual async::Future::Poll poll_read(container::Slice<uint8_t> &buf, size_t &s) throw(async::OsError) = 0;
	virtual bool				fail()																	   = 0;
	virtual ~ARequestBody() {}
};


class AResponseBody
{
public:
	virtual async::Future::Poll poll_write(container::Slice<uint8_t> buf, size_t &s) throw(async::OsError) = 0;
	virtual async::Future::Poll poll_end() throw(async::OsError)										   = 0;
	virtual ~AResponseBody() {}
};

class TestResponseBody : public AResponseBody
{
private:
	async::BufIo<> &io;

public:
	TestResponseBody(async::BufIo<> &io) : io(io) {}
	async::Future::Poll poll_write(container::Slice<uint8_t> buf, size_t &s) throw(async::OsError) { return io.poll_write(buf, s); }
	async::Future::Poll poll_end() throw(async::OsError) { return io.poll_flush(); }
	~TestResponseBody() {}
};

class ChunkedResponseBody : public AResponseBody
{
private:
	async::BufIo<>			 &io;
	size_t					  _chunk_written;
	container::Slice<uint8_t> _last_chunk;

public:
	ChunkedResponseBody(async::BufIo<> &io) : io(io), _last_chunk(const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>("0\r\n\r\n")), 5) {}
	~ChunkedResponseBody() {}
	async::Future::Poll poll_write(container::Slice<uint8_t> buf, size_t &s) throw(async::OsError)
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
		return io.poll_flush();
	}
	async::Future::Poll poll_flush() throw(async::OsError) { return io.poll_flush(); }
	async::Future::Poll poll_end() throw(async::OsError)
	{
		size_t s;
		if (_last_chunk.len() > 0 && io.poll_write(_last_chunk, s) == async::Future::Pending)
			return (async::Future::Pending);
		_last_chunk = _last_chunk.slice(s, _last_chunk.len());
		if (_last_chunk.len() == 0 && io.poll_flush() == async::Future::Pending)
			return (async::Future::Pending);
		return (async::Future::Ready);
	}
};

class TestRequestBody : public ARequestBody
{
private:
	bool			_fail;
	async::BufIo<> &io;
	size_t			_to_read;
	size_t			_read;

public:
	TestRequestBody(async::BufIo<> &io, size_t const &to_read) : _fail(false), io(io), _to_read(to_read), _read(0) {}
	~TestRequestBody() {}
	bool				fail() { return (false); };
	async::Future::Poll poll_read(container::Slice<uint8_t> &buf, size_t &s) throw(async::OsError)
	{
		if (_fail || _to_read == _read)
		{
			s = 0;
			return (async::Future::Ready);
		}
		size_t					  amt = std::min(buf.len(), _to_read - _read);
		container::Slice<uint8_t> tmp = buf.slice(0, amt);
		if (io.poll_read(tmp, s) == async::Future::Pending)
			return (async::Future::Pending);
		if (!s && _read != _to_read)
			_fail = true;
		_read += s;
		return async::Future::Ready;
	}
};

class ChunkedRequestBody : public ARequestBody
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
	ChunkedRequestBody(async::BufIo<> &io) :
		io(io), _buf(), _sl(_buf, sizeof(_buf)), _fail(false), _end(false), _chunk_read(0), _chunk_to_read(0), _buf_read(0), _buf_write(0)
	{
	}
	~ChunkedRequestBody() {}
	bool				fail() { return (_fail); };
	async::Future::Poll poll_read(container::Slice<uint8_t> &buf, size_t &call_written) throw(async::OsError)
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
		// while (_buf_write != 0)
		// {
		if (io.poll_read(_sl, _buf_write) == async::Future::Pending)
			return (async::Future::Ready);
		// }
		return (async::Future::Ready);
	}
};

class TestRequestBody : public ARequestBody
{
private:
	async::BufIo<> &io;

public:
	TestRequestBody(async::BufIo<> &io) : io(io) {}
	async::Future::Poll poll_read(container::Slice<uint8_t> &buf, size_t &s) throw(async::OsError) { return io.poll_read(buf, s); }
};

class HTTPStream : public async::Future
{
public:
	HTTPStream(async::Reactor &reactor, int fd, const struct sockaddr_storage &addr, socklen_t addr_len, Router *router);
	HTTPStream(HTTPStream const &other);
	~HTTPStream();

	HTTPStream	 &operator=(HTTPStream const &other);
	ARequestBody *request_body()
	{
		if (!request.peekHeader("Content-Length").empty())
		{
			char *end;
			return (new TestRequestBody(buf_io, std::strtol(request.peekHeader("Content-Length").c_str(), &end, 10)));
		}
		if (to_lower(request.peekHeader("Transfer-Encoding")) == "chunked")
			return (new ChunkedRequestBody(buf_io));
		return (NULL);
	}
	async::Future::Poll poll_send_headers(AResponseBody *&b)
	{
		this->response.headers.erase("Content-Length");
		response.addHeader("Transfer-Encoding", "chunked");
		if (this->sendHeaders() == Pending)
			return (Pending);
		if (response.content_length)
			b = NULL;
		else
			b = new ChunkedResponseBody(buf_io);
		return async::Future::Ready;
	}
	async::Future::Poll poll_send_headers(size_t content_length, AResponseBody *&b)
	{
		std::ostringstream oss;

		this->response.headers.erase("Transfer-Encoding");
		oss << content_length;
		this->response.addHeader("Content-Length", oss.str());
		if (this->sendHeaders() == Pending)
			return (Pending);
		if (response.content_length || content_length == 0)
			b = NULL;
		else
			b = new TestResponseBody(buf_io);
		return async::Future::Ready;
	}
	Future::Poll poll() throw(async::OsError);
	Future::Poll keepAlive();
	void		 setHTTPStatus(std::string const &status, std::string const &message);
	void		 setBrokenConnection();
	bool		 setStatusResponse();

	Response						   response;
	Request							   request;
	Router							  *router;
	struct sockaddr_storage			   addr;
	socklen_t						   addr_len;
	std::map<std::string, std::string> status_pages;


private:
	HTTPStream();
	void headersAddBase();
	Poll sendHeaders() throw(async::OsError);

	container::RingBuf<uint8_t, 4096> buf;

public: // FIXME: private
	async::BufIo<4096> buf_io;
	TestRequestBody	   _request_body;

private:
	bool			_override_response;
	bool			_rerun;
	bool			_broken_connection;
	std::string		_saved_status;
	std::string		_saved_msg;
	stream_status	_status;
	async::Reactor *_reactor;
	ABackend	   *_backend;
	headers_status	_headers_status;
	size_t			_headers_sent_bytes;
};

#endif
