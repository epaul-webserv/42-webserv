#ifndef INC_42_WEBSERV_STATICRESPONSEGENERATOR_HPP
#define INC_42_WEBSERV_STATICRESPONSEGENERATOR_HPP

#include <async>
#include "Request.hpp"
#include "Response.hpp"
#include "webserv.h"

class StaticResponseGenerator : public async::Future
{
public:
	StaticResponseGenerator(Request const &request, Response &response, async::Reactor &reactor, async::BufIo<> &bodyIo, int cgi_in_fd, int cgi_out_fd);
	Future::Poll poll() throw(async::OsError);
	~StaticResponseGenerator();

private:
	Request const  &_request;
	Response	   &_response;
	async::BufIo<> &_bodyIo;
	async::Io		_cgi_in;
	async::Io		_cgi_out;
	size_t			_body_len;

	bool _request_body_read_done;
	bool _body_write_done;
	bool _response_body_read_done;

	uint8_t _request_body_buf[4096];
	size_t	_request_body_read_len;
	size_t	_request_body_read_index;

	uint8_t _response_body_buf[4096];
	size_t	_response_body_read_len;
	size_t	_response_body_read_index;

	std::string cgi_body;
};


#endif
