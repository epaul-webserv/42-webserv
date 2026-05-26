#include "Request.hpp"
#include "webserv.h"
#include "string_overload.hpp"
#include "html_generator.hpp"

Request::Request(void) :
	_fd(-1), _first_line_read(false), _headers_read(false), _read_index(0),
	_write_index(0)
{
}

Request::Request(int fd) :
	_fd(fd), _first_line_read(false), _headers_read(false), _read_index(0),
	_write_index(0)
{
}

Request::Request(Request const &Request) :
	_fd(Request._fd),
	_first_line_read(Request._first_line_read),
	_headers_read(Request._headers_read),
	_read_index(0),
	_write_index(0),
	_method(Request._method),
	_URI(Request._URI),
	_http_version(Request._http_version),
	_headers(Request._headers)
{
}

Request::~Request(void)
{
}

Request const &Request::operator=(Request const &Request)
{
	this->_method          = Request._method;
	this->_URI             = Request._URI;
	this->_http_version    = Request._http_version;
	this->_fd              = Request._fd;
	this->_first_line_read = Request._first_line_read;
	this->_headers_read    = Request._headers_read;
	this->_headers         = Request._headers;
	this->_read_index      = Request._read_index;
	this->_write_index     = Request._write_index;
	return (*this);
}

void Request::clear(int fd)
{
	this->_method.clear();
	this->_URI.clear();
	this->_http_version.clear();
	this->_fd              = fd;
	this->_first_line_read = false;
	this->_headers_read    = false;
	this->_headers.clear();
	*this->_buffer     = '\0';
	this->_read_index  = 0;
	this->_write_index = 0;
}

void Request::prepareNextRead(void)
{
	this->_method.clear();
	this->_URI.clear();
	this->_http_version.clear();
	this->_first_line_read = false;
	this->_headers_read    = false;
	this->_headers.clear();
}

t_request_state Request::readRequest(void)
{
	size_t read_bytes = read(this->_fd, this->_buffer + this->_write_index,
							MAX_REQUEST_SIZE - this->_write_index);

	if (!read_bytes)
		return (_close);
	this->_buffer[read_bytes + this->_write_index] = '\0';
	this->_write_index                             += read_bytes;
	if (!_first_line_read)
		return (readFirstLine());
	return (readHeaders());
}

static size_t get_word(char *buffer, std::string &dest)
{
	size_t len     = 0;
	size_t skipped = 0;

	while (buffer[len] && std::isspace(buffer[len]))
		++buffer, ++skipped;
	while (buffer[len] && !std::isspace(buffer[len]))
		++len;
	dest.assign(buffer, len);
	return (len + skipped);
}

t_request_state Request::readFirstLine(void)
{
	if (std::strchr(this->_buffer + this->_read_index, '\n'))
	{
		this->_read_index += get_word(this->_buffer + this->_read_index,
									this->_method);
		this->_read_index += get_word(this->_buffer + this->_read_index,
									this->_URI);
		this->_read_index += get_word(this->_buffer + this->_read_index,
									this->_http_version);
		// if (!this->_URI.empty())
		//     this->_response = "HTTP/1.0 200 OK \r\n\r\ncoucou\r\n\r\n";
		this->_first_line_read = true;
		return (this->readHeaders());
	}
	return (_reading);
}

static size_t get_header(char *                             buffer,
						std::map<std::string, std::string> &headers)
{
	size_t                              whole_len = 0;
	size_t                              len       = 0;
	std::pair<std::string, std::string> new_pair;

	while (*buffer && std::isspace(buffer[len]))
	{
		++buffer;
		++whole_len;
	}
	while (buffer[len] && !std::isspace(buffer[len++]))
	{
		if (buffer[len] == ':')
		{
			new_pair.first.assign(buffer, len++);
			break ;
		}
	}
	if (new_pair.first.empty())
	{
		return (whole_len);
	}
	buffer    += len;
	whole_len += len;
	len       = 0;
	while (buffer[len] && std::isspace(buffer[len]))
	{
		++buffer;
		++whole_len;
	}
	while (buffer[len] && buffer[len] != '\r' && buffer[len] != '\n')
		++len;
	new_pair.second.assign(buffer, len);
	to_lower(new_pair.first);
	to_lower(new_pair.second);
	headers.insert(new_pair);
	return (whole_len + len);
}

t_request_state Request::readHeaders(void)
{
	while (this->_buffer[this->_read_index] &&
			std::strcmp(this->_buffer + this->_read_index, "\r\n\r\n") &&
			std::strcmp(this->_buffer + this->_read_index, "\r\n\r\n") &&
			std::strstr(this->_buffer, "\n"))
		this->_read_index += get_header(this->_buffer + this->_read_index,
										this->_headers);
	if (std::strstr(this->_buffer, "\r\n\r\n") || std::strstr(
			this->_buffer, "\n\n"))
		return (_read);
	return (_reading);
}

# include <fstream>
# include <iostream>
# include <fcntl.h>
# include <unistd.h>
# include <sys/stat.h>

// bool Request::sendResponse(void)
// {
// 	return this->_response.sendResponse();
// }
//
// std::string Request::getFirstLine(void) const
// {
// 	return (this->_method + " " + this->_URI + " " + this->_http_version);
// }
//
// void Request::prepareResponse(void)
// {
// 	decodeURL(this->_URI);
// 	if (this->_URI.empty())
// 	{
// 		this->_http_version = "HTTP/1.1";
// 		this->createErrorResponse("400", "Bad Request");
// 	}
// 	else if (this->_http_version.empty())
// 		this->response_0_9();
// 	else if (this->_http_version == "HTTP/1.0")
// 		this->response_1_0();
// 	else if (this->_http_version == "HTTP/1.1")
// 		this->response_1_1();
// 	else
// 	{
// 		if (this->_http_version.find("HTTP/") == std::string::npos)
// 		{
// 			this->_http_version = "HTTP/1.1";
// 			this->createErrorResponse("400", "Bad Request");
// 		}
// 		else
// 		{
// 			this->_http_version = "HTTP/1.1";
// 			this->createErrorResponse("505", "HTTP Version Not Supported");
// 		}
// 	}
// 	std::cout << this->_method << " " << this->_URI << " " << this->
// 		_http_version << std::endl;
// }
//
// void Request::response_0_9(void)
// {
// 	if (this->_method == "GET")
// 	{
// 		switch (fileType(this->_URI, DEFAULT_ROOT))
// 		{
// 		case _dir:
// 			return this->_response.bodyAppend(
// 				get_html_dir(this->_URI, DEFAULT_ROOT));
// 		case _no_access:
// 			return this->_response.bodyAppend(
// 				get_html_error("405", "Forbidden"));
// 		case _none:
// 			return this->_response.bodyAppend(
// 				get_html_error("404", "Not found"));
// 		case _err:
// 			return this->_response.bodyAppend(
// 				get_html_error("500", "Internal Server Error"));
// 		case _file:
// 			break;
// 		}
// 		switch (this->_response.openFile(DEFAULT_ROOT + this->_URI))
// 		{
// 		case _no_permission:
// 			return this->_response.bodyAppend(
// 				get_html_error("405", "Forbidden"));
// 		default:
// 			return;
// 		}
// 	}
// 	else
// 		this->_response.headAppend(get_html_error("405", "Method not allowed"));
// }
//
// void Request::response_1_0(void)
// {
// 	if (this->_method == "GET")
// 	{
// 		switch (fileType(this->_URI, DEFAULT_ROOT))
// 		{
// 		case _dir:
// 			this->_response.headAppend(this->_http_version + " 200 OK\r\n");
// 			this->_response.bodyAppend(get_html_dir(this->_URI, DEFAULT_ROOT));
// 			return this->_response.setHeaders();
// 		case _no_access:
// 			return this->createErrorResponse("403", "Forbidden");
// 		case _none:
// 			return this->createErrorResponse("405", "Not Found");
// 		case _err:
// 			return this->createErrorResponse("500", "Internal Server Error");
// 		case _file:
// 			break;
// 		}
// 		switch (this->_response.openFile(DEFAULT_ROOT + this->_URI))
// 		{
// 		case _no_permission:
// 			return this->createErrorResponse("403", "Forbidden");
// 		default:
// 			this->_response.headAppend(this->_http_version + " 200 OK\r\n");
// 			this->_response.setHeaders();
// 			return;
// 		}
// 	}
// 	// if (this->_method == "HEAD")
// 	//     return (this->HEAD());
// 	// if (this->_method == "POST")
// 	//     return (this->POST());
// 	else
// 	{
// 		this->_response.headAppend(
// 			this->_http_version + " 405 Method Not Allowed\r\n");
// 		this->_response.setHeaders();
// 		return this->_response.bodyAppend(
// 			get_html_error("405", "Method Not Allowed"));
// 	}
// }
//
// void Request::response_1_1(void)
// {
// 	if (this->_method == "GET")
// 	{
// 		switch (fileType(this->_URI, DEFAULT_ROOT))
// 		{
// 		case _dir:
// 			this->_response.headAppend(this->_http_version + " 200 OK\r\n");
// 			this->_response.bodyAppend(get_html_dir(this->_URI, DEFAULT_ROOT));
// 			return this->_response.setHeaders();
// 		case _no_access:
// 			return this->createErrorResponse("403", "Forbidden");
// 		case _none:
// 			return this->createErrorResponse("404", "Not Found");
// 		case _err:
// 			return this->createErrorResponse("500", "Internal Server Error");
// 		case _file:
// 			break;
// 		}
// 		switch (this->_response.openFile(DEFAULT_ROOT + this->_URI))
// 		{
// 		case _no_permission:
// 			return this->createErrorResponse("403", "Forbidden");
// 		default:
// 			this->_response.headAppend(this->_http_version + " 200 OK\r\n");
// 			this->_response.setHeaders();
// 			return;
// 		}
// 	}
// 	// if (this->_method == "HEAD")
// 	//     return (this->HEAD());
// 	// if (this->_method == "POST")
// 	//     return (this->POST());
// 	// if (this->_method == "OPTIONS")
// 	//     return (this->OPTIONS());
// 	// if (this->_method == "PUT")
// 	//     return (this->PUT());
// 	// if (this->_method == "DELETE")
// 	//     return (this->DELETE());
// 	// if (this->_method == "TRACE")
// 	//     return (this->TRACE());
// 	// if (this->_method == "CONNECT")
// 	//     return (this->CONNECT());
// 	else
// 	{
// 		this->_response.headAppend(
// 			this->_http_version + " 405 Method Not Allowed\r\n");
// 		this->_response.setHeaders();
// 		return this->_response.bodyAppend(
// 			get_html_error("405", "Method Not Allowed"));
// 	}
// }
//
// void Request::createErrorResponse(std::string const &statusCode,
// 								std::string const &  errorMessage)
// {
// 	this->_response.headAppend(
// 		this->_http_version + " " + statusCode + " " + errorMessage + "\r\n");
// 	this->_response.bodyAppend(get_html_error(statusCode, errorMessage));
// 	this->_response.setHeaders();
// }

/// @brief returns the next header available.
/// This function does not delete the returned header
/// (use getHeader for deletion).
/// @param key Key of the requested header, do not send to get the next header
/// @return value of the requested header or empty string if it does not exist
std::pair<std::string, std::string> Request::peekHeader(std::string const key)
{
	std::pair<std::string, std::string>          ret;
	std::map<std::string, std::string>::iterator it;

	if (key.empty())
		it = this->_headers.begin();
	else
		it = this->_headers.find(key);
	ret = *it;
	return (ret);
}

/// @brief returns the next header available.
/// This function deletes the returned header
/// (use peekHeader if this behavior is not desired).
/// @param key Key of the requested header, do not send to get the next header
/// @return value of the requested header or empty string if it does not exist
std::pair<std::string, std::string> Request::getHeader(std::string const key)
{
	std::pair<std::string, std::string> ret = peekHeader(key);

	this->_headers.erase(key);
	return (ret);
}

std::string Request::getMethod(void)
{
	return (this->_method);
}

std::string Request::getURI(void)
{
	return (this->_URI);
}

std::string Request::getHTTPVersion(void)
{
	return (this->_http_version);
}

const char *Request::InvalidReadException::what() const throw()
{
	return ("Error while reading");
}
