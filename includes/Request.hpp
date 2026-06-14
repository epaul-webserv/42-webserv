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
	Request();
	Request(const Request &Request);
	~Request();

	Request		   &operator=(const Request &Request);
	void			clear();
	void			prepareNextRead();
	t_request_state readRequest(async::BufIo<> &client_io);
	std::string		peekHeader(std::string const &key = "") const;
	std::string		getHeader(std::string const &key = "");
	std::string		getMethod() const;
	std::string		getURI() const;
	void			setURI(std::string const &new_URI);
	std::string		getQueryString() const;
	std::string		getHTTPVersion() const;

	class InvalidReadException : public std::exception
	{
		const char *what() const throw();
	};

	t_request_state readFirstLine(async::BufIo<> &client_io);
	t_request_state readHeaders(async::BufIo<> &client_io);

	bool							   _first_line_read;
	std::string						   method;
	std::string						   URI;
	std::string						   query_string;
	std::string						   http_version;
	std::map<std::string, std::string> headers;
	size_t							   _body_read_index;
};

#endif
