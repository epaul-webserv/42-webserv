#include <async>
#include "Request.hpp"
#include "Response.hpp"
#include "webserv.h"

class CGIResponseGenerator : public async::Future
{
public:
	CGIResponseGenerator(Request const &request, Response &response, async::Reactor &reactor, async::BufIo<> &bodyIo, int cgi_in_fd, int cgi_out_fd);
	Future::Poll poll() throw(async::OsError);
	~CGIResponseGenerator();

private:
	CGIResponseGenerator(const CGIResponseGenerator &other);
	CGIResponseGenerator &operator=(const CGIResponseGenerator &other);
	Request const		 &_request;
	Response			 &_response;
	async::BufIo<>		 &_bodyIo;
	async::Io			  _cgi_in;
	async::Io			  _cgi_out;
	size_t				  _body_len;


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
