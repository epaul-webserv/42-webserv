#include <async>
#include "Request.hpp"
#include "Response.hpp"
#include "webserv.h"

class CGIResponseGenerator : public async::Future
{
public:
	CGIResponseGenerator(Request const &request, Response &response, async::Reactor &reactor, async::BufIo<> &bodyIo, int cgi_io_fd);
	Future::Poll poll() throw(async::OsError);
	~CGIResponseGenerator();

private:
	CGIResponseGenerator(const CGIResponseGenerator &other);
	CGIResponseGenerator &operator=(const CGIResponseGenerator &other);
	bool				  parse_cgi_header(const std::vector<uint8_t> &raw_line, std::string &key, std::string &value);
	void				  parse_cgi_status_value(const std::string &status_value, std::string &status_code, std::string &status_message);
	Request const		 &_request;
	Response			 &_response;
	async::BufIo<>		 &_bodyIo;
	async::BufIo<>		 *_cgi_io;
	size_t				  _body_len;
	std::string			  headers;


	bool _request_body_read_done;
	bool _body_write_done;
	bool _response_headers_read_done;
	bool _status_set;

	uint8_t _request_body_buf[4096];
	size_t	_request_body_read_len;
	size_t	_request_body_read_index;

	uint8_t _response_body_buf[4096];
	size_t	_response_body_read_len;
	size_t	_response_body_read_index;

	std::string cgi_body;
};
