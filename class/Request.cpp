#include "Request.hpp"
#include "html_generator.hpp"
#include "string_overload.hpp"
#include "url_utils.hpp"
#include "webserv.h"

Request::Request() : _first_line_read(false), _body_read_index(0) {}

Request::Request(Request const &Request) :
	_first_line_read(Request._first_line_read), method(Request.method), URI(Request.URI), http_version(Request.http_version), headers(Request.headers),
	_body_read_index(0)
{
}

Request::~Request() {}

Request &Request::operator=(Request const &Request)
{
	this->method		   = Request.method;
	this->URI			   = Request.URI;
	this->http_version	   = Request.http_version;
	this->_first_line_read = Request._first_line_read;
	this->headers		   = Request.headers;
	this->_body_read_index = Request._body_read_index;
	return (*this);
}

void Request::clear()
{
	this->_first_line_read = false;
	this->method.clear();
	this->URI.clear();
	this->query_string.clear();
	this->http_version.clear();
	this->headers.clear();
	this->_body_read_index = 0;
}

void Request::prepareNextRead()
{
	this->method.clear();
	this->URI.clear();
	this->http_version.clear();
	this->_first_line_read = false;
	this->headers.clear();
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
	while (!line.empty() && line[len] && std::isspace(line[len]))
		line.erase(line.begin()), ++skipped;
	if (line.empty())
		return (skipped);
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

	if (line.empty() || line.size() == 0)
		return (dest);
	while (!line.empty() && line[len] && std::isspace(line[len]))
		line.erase(line.begin()), ++skipped;
	if (line.empty())
		return (dest);
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
		if (line.empty())
			return (_close);
		get_next_word(line, this->method);
		if (this->method.empty())
			return (_read);
		get_next_word(line, this->URI);
		if (this->URI.empty())
			return (_read);
		get_next_word(line, this->http_version);
		this->_first_line_read = true;
		decodeURL(this->URI);
		if (this->URI.find('?') != std::string::npos)
		{
			this->query_string = this->URI.substr(this->URI.find('?'));
			this->URI		   = this->URI.substr(0, this->URI.find('?'));
		}
		return (this->readHeaders(client_io));
	}
	return (_reading);
}

t_request_state Request::readHeaders(async::BufIo<> &client_io)
{
	std::vector<uint8_t> line;

	while (client_io.poll_read_line(line) == async::Future::Ready)
	{
		if (line.empty())
			return (_close);
		std::vector<uint8_t>::iterator ite = line.end();
		if ((line.size() == 2 && line[0] == '\r') || (line.size() == 1 && line[0] == '\n'))
			return (_read);
		std::vector<uint8_t>::iterator it		  = std::find(line.begin(), line.end(), ':');
		std::string					   header_key = get_next_word(line);
		if (header_key.find(':') == std::string::npos)
		{
			this->headers.insert((std::pair<std::string, std::string>(header_key, "")));
			line.clear();
			break;
		}
		header_key = header_key.substr(0, header_key.find(':'));
		if (it != ite)
			this->headers.insert((std::pair<std::string, std::string>(to_lower(header_key), get_end_of_line(line))));
		line.clear();
	}
	return (_reading);
}

/// @brief returns the next header available.
/// This function does not delete the returned header
/// (use getHeader for deletion).
/// @param key Key of the requested header, do not send to get the next header
/// @return value of the requested header or empty string if it does not exist
std::string Request::peekHeader(std::string const &key) const
{
	std::map<std::string, std::string>::const_iterator it;

	if (key.empty())
		it = this->headers.begin();
	else
	{
		std::string tmp = key;
		to_lower(tmp);
		it = this->headers.find(tmp);
	}
	if (it == this->headers.end())
		return ("");
	return (it->second);
}

/// @brief returns the next header available.
/// This function deletes the returned header
/// (use peekHeader if this behavior is not desired).
/// @param key Key of the requested header, do not send to get the next header
/// @return value of the requested header or empty string if it does not exist
std::string Request::getHeader(std::string const &key)
{
	std::string ret = peekHeader(key);

	this->headers.erase(key);
	return (ret);
}

std::string Request::getMethod() const { return (this->method); }

std::string Request::getURI() const { return (this->URI); }

void Request::setURI(std::string const &new_URI) { this->URI = new_URI; }

std::string Request::getQueryString() const { return (this->query_string); }

std::string Request::getHTTPVersion() const { return (this->http_version); }

const char *Request::InvalidReadException::what() const throw() { return ("Error while reading"); }
