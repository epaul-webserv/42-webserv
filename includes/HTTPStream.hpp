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
	_request,
	_response_pending,
	_response_sending
};

class HTTPStream : public async::Future
{
public:
	HTTPStream(async::Reactor &reactor, int fd, struct sockaddr_storage addr, socklen_t addr_len, Router *router);
	HTTPStream(HTTPStream const &HTTPStream);
	~HTTPStream();

	HTTPStream	&operator=(HTTPStream const &other);
	Future::Poll poll() throw(async::OsError);
	Future::Poll keepAlive(void);

	Response				response;
	Request					request;
	Router				   *router;
	struct sockaddr_storage addr;
	socklen_t				addr_len;

private:
	HTTPStream(void);

	container::RingBuf<uint8_t, 4096> buf;
	async::Io						  io;
	async::BufIo<4096>				  buf_io;
	stream_status					  _status;
	async::Reactor					 *_reactor;
};

#endif
