#pragma once

#include <async>
#include <vector>
#include "ABackend.hpp"

class CGIBackend : public ABackend
{
public:
	CGIBackend(std::vector<std::string> const &args, Request const &request, std::string const &root, Response &response, async::Reactor &reactor, async::BufIo<> &io,
			   std::map<std::string, std::string> &status_pages);
	~CGIBackend(void);

	// void generateResponse(Request const &request, Response &response, async::Reactor &reactor, async::BufIo<> &io);
	Poll poll();

private:
	enum status
	{
		_init,
		_processing
	};
	CGIBackend(const CGIBackend &other);
	CGIBackend &operator=(const CGIBackend &other);


	bool					 parse_cgi_header(const std::vector<uint8_t> &raw_line, std::string &key, std::string &value);
	void					 parse_cgi_status_value(const std::string &status_value, std::string &status_code, std::string &status_message);
	void					 init();
	async::Future::Poll		 process_io() throw(async::OsError);
	std::vector<std::string> _args;
	async::BufIo<>			*_cgi_io;
	size_t					 _body_len;
	std::string				 headers;


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

	status _status;
};
