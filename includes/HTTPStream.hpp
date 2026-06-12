#ifndef WEBSERV_HTTPSTREAM_HPP
#define WEBSERV_HTTPSTREAM_HPP

#include <async>
#include <iostream>
#include <sys/socket.h>
#include "Request.hpp"
#include "Response.hpp"
#include "Router.hpp"

enum stream_status
{
	_fetching_request,
	_getting_backend,
	_processing_response,
	_sending_response
};


class ARequestBody
{
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

class HTTPStream : public async::Future
{
public:
	HTTPStream(async::Reactor &reactor, int fd, struct sockaddr_storage addr, socklen_t addr_len, Router *router);
	HTTPStream(HTTPStream const &HTTPStream);
	~HTTPStream();

	HTTPStream		   &operator=(HTTPStream const &other);
	ARequestBody	   *request_body();
	async::Future::Poll poll_send_headers(AResponseBody *&b)
	{
		// FIXME: send headers
		b = new TestResponseBody(buf_io);
		return async::Future::Ready;
	}
	async::Future::Poll poll_send_headers(size_t content_length, AResponseBody *&b)
	{
		// FIXME: send headers
		b = new TestResponseBody(buf_io);
		return async::Future::Ready;
	}
	Future::Poll poll() throw(async::OsError);
	Future::Poll keepAlive(void);

	Response							response;
	Request								request;
	Router							   *router;
	struct sockaddr_storage				addr;
	socklen_t							addr_len;
	std::map<std::string, std::string> *status_pages;


private:
	HTTPStream(void);

	container::RingBuf<uint8_t, 4096> buf;

public: // FIXME: private
	async::BufIo<4096> buf_io;

private:
	stream_status	_status;
	async::Reactor *_reactor;
	ABackend	   *_backend;
};

#endif
