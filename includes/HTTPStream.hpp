#ifndef WEBSERV_HTTPSTREAM_HPP
#define WEBSERV_HTTPSTREAM_HPP

#include "Request.hpp"
#include "Response.hpp"

class HTTPStream
{
public:
	HTTPStream(void);
	HTTPStream(int fd);
	~HTTPStream();

	void			clear(int fd);
	t_request_state readRequest(void);
	bool			sendResponse(void);

	Response response;
	Request	 request;

private:
	int _fd;
};

#endif
