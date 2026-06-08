#include "Request.hpp"
#include "html_generator.hpp"
#include "string_overload.hpp"
#include "url_utils.hpp"
#include "webserv.h"

Request::Request(void) : _body_read_index(0), _first_line_read(false) {}

Request::Request(Request const &Request) :
	_body_read_index(0), _first_line_read(Request._first_line_read), _method(Request._method), _URI(Request._URI), _http_version(Request._http_version),
	_headers(Request._headers)
{
}

Request::~Request(void) {}

Request const &Request::operator=(Request const &Request)
{
	this->_method		   = Request._method;
	this->_URI			   = Request._URI;
	this->_http_version	   = Request._http_version;
	this->_first_line_read = Request._first_line_read;
	this->_headers		   = Request._headers;
	this->_body_read_index = Request._body_read_index;
	return (*this);
}

void Request::clear()
{
	this->_method.clear();
	this->_URI.clear();
	this->_http_version.clear();
	this->_first_line_read = false;
	this->_headers.clear();
	*this->_buffer		   = '\0';
	this->_body_read_index = 0;
}

void Request::prepareNextRead(void)
{
	this->_method.clear();
	this->_URI.clear();
	this->_http_version.clear();
	this->_first_line_read = false;
	this->_headers.clear();
}

t_request_state Request::readRequest(async::BufIo<> &client_io)
{
	if (!this->_first_line_read)
		return (readFirstLine(client_io));
	return (readHeaders(client_io));
}

static size_t get_next_word(std::vector<uint8_t> &line, std::string &dest)
{
	size_t len	   = 0;
	size_t skipped = 0;

	if (line.empty())
		return (0);
	while (line[len] && std::isspace(line[len]))
		line.erase(line.begin()), ++skipped;
	while (line[len] && !std::isspace(line[len]))
		++len;
	dest = std::string(&line[0], &line[len]);
	line.erase(line.begin(), line.begin() + len);
	return (len + skipped);
}

static std::string get_next_word(std::vector<uint8_t> &line)
{
	size_t		len		= 0;
	size_t		skipped = 0;
	std::string dest;

	if (line.empty())
		return (dest);
	while (line[len] && std::isspace(line[len]))
		line.erase(line.begin()), ++skipped;
	while (line[len] && !std::isspace(line[len]))
		++len;
	dest = std::string(&line[0], &line[len]);
	line.erase(line.begin(), line.begin() + len);
	return (dest);
}

static std::string get_end_of_line(std::vector<uint8_t> &line)
{
	size_t		len		= 0;
	size_t		skipped = 0;
	std::string dest;

	if (line.empty())
		return (dest);
	while (line[len] && std::isspace(line[len]))
		line.erase(line.begin()), ++skipped;
	while (line[len] && line[len] != '\r' && line[len] != '\n')
		++len;
	dest = std::string(&line[0], &line[len]);
	line.erase(line.begin(), line.begin() + len);
	return (dest);
}

t_request_state Request::readFirstLine(async::BufIo<> &client_io)
{
	std::vector<uint8_t> line;

	if (client_io.poll_read_line(line) == async::Future::Ready)
	{
		if (line.size() == 0)
			return (_close);
		get_next_word(line, this->_method);
		if (this->_method.empty())
			return (_read);
		get_next_word(line, this->_URI);
		get_next_word(line, this->_http_version);
		this->_first_line_read = true;
		decodeURL(this->_URI);
		if (this->_URI.find('?') != std::string::npos)
		{
			this->_query_string = this->_URI.substr(this->_URI.find('?'));
			this->_URI			= this->_URI.substr(0, this->_URI.find('?'));
		}
		return (this->readHeaders(client_io));
	}
	return (_reading);
}

t_request_state Request::readHeaders(async::BufIo<> &client_io)
{
	std::vector<uint8_t>		   line;
	std::vector<uint8_t>::iterator it;
	std::vector<uint8_t>::iterator ite;

	while (client_io.poll_read_line(line) == async::Future::Ready)
	{
		if (line.size() == 0)
			return (_close);
		ite = line.end();
		if (line.size() == 2 && line[0] == '\r')
			return (_read);
		it					   = std::find(line.begin(), line.end(), ':');
		std::string header_key = get_next_word(line);
		if (header_key.find(':') == std::string::npos)
		{
			this->_headers.insert((std::pair<std::string, std::string>(header_key, "")));
			line.clear();
			break;
		}
		header_key = header_key.substr(0, header_key.find(':'));
		if (it != ite)
			this->_headers.insert((std::pair<std::string, std::string>(to_lower(header_key), get_end_of_line(line))));
		line.clear();
	}
	return (_reading);
}

/// @brief returns the next header available.
/// This function does not delete the returned header
/// (use getHeader for deletion).
/// @param key Key of the requested header, do not send to get the next header
/// @return value of the requested header or empty string if it does not exist
std::string Request::peekHeader(std::string const key) const
{
	std::map<std::string, std::string>::const_iterator it;

	if (key.empty())
		it = this->_headers.begin();
	else
	{
		std::string tmp = key;
		to_lower(tmp);
		it = this->_headers.find(tmp);
	}
	if (it == this->_headers.end())
		return ("");
	return (it->second);
}

/// @brief returns the next header available.
/// This function deletes the returned header
/// (use peekHeader if this behavior is not desired).
/// @param key Key of the requested header, do not send to get the next header
/// @return value of the requested header or empty string if it does not exist
std::string Request::getHeader(std::string const key)
{
	std::string ret = peekHeader(key);

	this->_headers.erase(key);
	return (ret);
}

std::string Request::getMethod(void) const { return (this->_method); }

std::string Request::getURI(void) const { return (this->_URI); }

std::string Request::getQueryString(void) const { return (this->_query_string); }

std::string Request::getHTTPVersion(void) const { return (this->_http_version); }

const char *Request::InvalidReadException::what() const throw() { return ("Error while reading"); }
