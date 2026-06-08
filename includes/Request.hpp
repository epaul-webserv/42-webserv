#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <algorithm>
#include <async>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "default_conf.hpp"
#include "string_overload.hpp"

typedef enum
{
	_reading,
	_read,
	_close
} t_request_state;

typedef enum
{
	_dir,
	_file,
	_no_access,
	_none,
	_err
} t_file_type;

class Request
{
public:
	Request(void);
	Request(const Request &Request);
	Request(int fd);
	~Request(void);

	Request const  &operator=(const Request &Request);
	void			clear();
	void			prepareNextRead(void);
	t_request_state readRequest(char *buffer, size_t read_bytes);
	t_request_state readRequest(async::BufIo<> &client_io);
	std::string		peekHeader(std::string const key = "") const;
	std::string		getHeader(std::string const key = "");
	std::string		getMethod(void) const;
	std::string		getURI(void) const;
	std::string		getQueryString(void) const;
	std::string		getHTTPVersion(void) const;

	class InvalidReadException : public std::exception
	{
		const char *what(void) const throw();
	};

	size_t _body_read_index;

private:
	t_request_state readFirstLine(void);
	t_request_state readHeaders(void);
	t_request_state readFirstLine(async::BufIo<> &client_io);
	t_request_state readHeaders(async::BufIo<> &client_io);

	bool _first_line_read;
	char _buffer[MAX_REQUEST_SIZE + 1];
	// size_t							   _write_index;
	std::string						   _method;
	std::string						   _URI;
	std::string						   _query_string;
	std::string						   _http_version;
	std::map<std::string, std::string> _headers;
	// StaticResponse                     _response;
};

#endif
