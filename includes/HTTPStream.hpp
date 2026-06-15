#ifndef WEBSERV_HTTPSTREAM_HPP
#define WEBSERV_HTTPSTREAM_HPP

#include <async>
#include <iostream>
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
};


class AResponseBody
{
public:
	virtual async::Future::Poll poll_write(container::Slice<uint8_t> buf, size_t &s) throw(async::OsError) = 0;
	virtual async::Future::Poll poll_flush() throw(async::OsError)										   = 0;
};

class TestResponseBody : public AResponseBody
{
private:
	async::BufIo<> &io;

public:
	TestResponseBody(async::BufIo<> &io) : io(io) {}
	async::Future::Poll poll_write(container::Slice<uint8_t> buf, size_t &s) throw(async::OsError) { return io.poll_write(buf, s); }
	async::Future::Poll poll_flush() throw(async::OsError) { return io.poll_flush(); }
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

	HTTPStream		   &operator=(HTTPStream const &other);
	ARequestBody	   *request_body();
	async::Future::Poll poll_send_headers(AResponseBody *&b)
	{
		// FIXME: chunk body
		if (this->sendHeaders() == Pending)
			return (Pending);
		if (response.content_length)
			b = NULL;
		else
			b = new TestResponseBody(buf_io);
		return async::Future::Ready;
	}
	async::Future::Poll poll_send_headers(size_t content_length, AResponseBody *&b)
	{
		std::ostringstream oss;

		oss << content_length;
		this->response.addHeader("Content-Length", oss.str());
		if (this->sendHeaders() == Pending)
			return (Pending);
		if (response.content_length)
			b = NULL;
		else
			b = new TestResponseBody(buf_io);
		return async::Future::Ready;
	}
	Future::Poll poll() throw(async::OsError);
	Future::Poll keepAlive();
	void		 setHTTPStatus(std::string const &status, std::string const &message);
	void		 setBrokenConnection();

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
	bool			_broken_connection;
	stream_status	_status;
	async::Reactor *_reactor;
	ABackend	   *_backend;
	headers_status	_headers_status;
	size_t			_headers_sent_bytes;
};

#endif
