#include "Response.hpp"
#include <async>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <strings.h>
#include <sys/mman.h>
#include <unistd.h>
#include "CGIResponseGenerator.hpp"
#include "webserv.h"

Response::Response(void) :
	_send_status(_first_step), _fetch_status(_init), _file_fd(-1), _header_read_index(0), _header_write_index(0), _body_read_index(0), _body_write_index(0),
	_content_length(0), _sent_bytes(0), _map(NULL)
{
	bzero(&this->_sb, sizeof(this->_sb));
}

Response::Response(Response const &Response) :
	_send_status(Response._send_status), _fetch_status(Response._fetch_status), _file_fd(Response._file_fd), _head(Response._head), _body(Response._body),
	_header_read_index(Response._header_read_index), _header_write_index(Response._header_write_index), _body_read_index(Response._body_read_index),
	_body_write_index(Response._body_write_index), _content_length(Response._content_length), _sent_bytes(Response._sent_bytes), _http_version(Response._http_version),
	_http_status(Response._http_status), _sb(Response._sb), _map(Response._map), _response_generator(NULL)
{
}

Response::~Response(void)
{
	if (this->_map)
		munmap(this->_map, this->_sb.st_size);
	if (this->_file_fd != -1)
		close(this->_file_fd);
	if (_response_generator)
		delete _response_generator;
}

void Response::clear()
{
	if (this->_file_fd != -1)
		close(this->_file_fd);
	if (this->_map)
		munmap(this->_map, this->_sb.st_size);
	this->_send_status	= _first_step;
	this->_fetch_status = _init;
	this->_map			= NULL;
	this->_head.clear();
	this->_body.clear();
	this->_header_read_index  = 0;
	this->_header_write_index = 0;
	this->_body_read_index	  = 0;
	this->_body_write_index	  = 0;
	this->_sent_bytes		  = 0;
	this->_content_length	  = 0;
	bzero(&this->_sb, sizeof(this->_sb));
	if (_response_generator)
		delete _response_generator;
	_response_generator = NULL;
}

bool Response::ready(void)
{
	if (this->_fetch_status == _ready)
		return (true);
	return (false);
}

void Response::headAppend(std::string const &content)
{
	if (content.empty())
		return;
	this->_head.append(content);
	if (*(content.end() - 1) != '\n')
		this->_head.push_back('\n');
}

void Response::addHeader(std::string const &key, std::string const &value) { this->_head.append(key + ": " + value + "\r\n"); }

std::string const &Response::getHTTPVersion(void) { return (this->_http_version); }

void Response::setHTTPVersion(std::string const &http_version) { this->_http_version = http_version; }

void Response::setHTTPStatus(std::string const &status, std::string const &message) { this->_http_status = status + " " + message; }

void Response::setHTTPStatus(std::string const &completeStatus) { this->_http_status = completeStatus; }

void Response::bodyAppend(std::string const &content)
{
	this->_body.append(content);
	this->_content_length += content.size();
}

t_file_status Response::openFile(std::string const &path)
{
	if (this->_map)
		return (_already_open);
	if (stat((path).c_str(), &this->_sb) == -1)
		return (_not_found);
	this->_file_fd = open((path).c_str(), O_RDONLY);
	if (this->_file_fd < 0)
	{
		if (errno == EACCES)
			return (_no_permission);
		return (_no_permission);
	}
	// if (!this->_sb.st_size)
	//     return (_open);
	this->_map = mmap(NULL, this->_sb.st_size, PROT_READ, MAP_PRIVATE, this->_file_fd, 0);
	if (this->_map == MAP_FAILED)
	{
		close(this->_file_fd);
		this->_file_fd = -1;
		this->_map	   = NULL;
		return (_not_found);
	}
	return (_open);
}

void Response::setHttpVersion(std::string const &http_version)
{
	if (http_version != "HTTP/1.0" && http_version != "HTTP/1.1")
	{
		throw(std::runtime_error("Invalid HTTP version"));
	}
	this->_http_version = http_version;
}

t_fetch_status Response::getStatus(void) { return (this->_fetch_status); }

std::string const &Response::getHTTPStatus(void) { return (this->_http_status); }

void Response::setStatus(t_fetch_status status) { this->_fetch_status = status; }

void Response::setGenerator(Request const &request, Response &response, async::Reactor &reactor, async::Io bodyIo, int cgi_in_fd, int cgi_out_fd)
{
	_response_generator = new CGIResponseGenerator(request, response, reactor, bodyIo, cgi_in_fd, cgi_out_fd);
}

void Response::pollGenerator()
{
	if (_response_generator && _response_generator->poll() == async::Future::Ready)
		setStatus(_ready);
}

void Response::addBase()
{
	std::ostringstream head;

	if (this->_http_version == "HTTP/0.9")
		return;
	head << this->_http_version + " " + this->_http_status << "\r\n";
	head << "Server: " << PROGNAME << "\r\n";
	head << "Date: " << Date().str() << "\r\n";
	if (this->_map || this->_content_length)
		head << "Content-Length: " << (this->_map ? this->_sb.st_size : this->_content_length) << "\r\n";
	this->_head.insert(0, head.str());
	this->_head.append("\r\n");
}

void Response::sendHead(int fd)
{
	int sent_bytes = send(fd, this->_head.c_str() + this->_header_read_index, this->_head.size() - this->_header_read_index, 0);

	if (sent_bytes == -1)
		return;
	this->_header_read_index += sent_bytes;
	if (this->_header_read_index == this->_head.size())
		this->_send_status = _sending_body;
}

bool Response::sendBody(int fd)
{
	char const *buffer;
	size_t		to_read;

	if (this->_map)
	{
		buffer	= (char *)this->_map;
		to_read = this->_sb.st_size;
	}
	else if (!this->_body.empty())
	{
		buffer	= this->_body.c_str();
		to_read = this->_body.size();
	}
	else
		return (this->_send_status = _body_sent);
	int sent_bytes = send(fd, buffer + this->_sent_bytes, to_read - this->_sent_bytes, MSG_NOSIGNAL);

	if (sent_bytes == -1)
		return (true);
	this->_sent_bytes += sent_bytes;
	if (this->_sent_bytes == to_read)
	{
		if (this->_map)
		{
			munmap(this->_map, this->_sb.st_size);
			this->_map = NULL;
			close(this->_file_fd);
			this->_file_fd = -1;
		}
		this->_send_status = _body_sent;
		return (true);
	}
	return (false);
}

bool Response::sendResponse(int fd)
{

	if (this->_send_status == _first_step)
	{
		this->addBase();
		this->_send_status = _sending_head;
	}
	if (this->_send_status == _sending_head)
		return (this->sendHead(fd), false);
	if (this->_send_status == _sending_body)
		return (this->sendBody(fd));
	return (true);
}

void Response::sendHead(container::Slice<uint8_t> &slice)
{
	if (this->_send_buf.size() - this->_header_read_index > 0)
		slice = container::Slice<uint8_t>(&this->_send_buf[this->_header_read_index], this->_head.size() - this->_header_read_index);
}

void Response::sendBody(container::Slice<uint8_t> &slice)
{
	uint8_t *buffer	 = 0;
	size_t	 to_read = 0;

	if (this->_map)
	{
		buffer	= (uint8_t *)this->_map;
		to_read = this->_sb.st_size - this->_body_read_index;
	}
	else if (this->_send_buf.size() > this->_header_read_index + this->_body_read_index)
	{
		buffer	= &this->_send_buf[this->_header_read_index + this->_body_read_index];
		to_read = this->_body.size() - this->_body_read_index;
	}
	slice = container::Slice<uint8_t>(buffer, to_read);
}

void Response::sendResponse(container::Slice<uint8_t> &slice)
{
	if (this->_send_status == _first_step)
	{
		this->addBase();
		this->_send_buf = std::vector<uint8_t>(this->_head.c_str(), this->_head.c_str() + this->_head.size());
		this->_send_buf.insert(this->_send_buf.end(), this->_body.begin(), this->_body.end());
		this->_send_buf.push_back('\0');
		this->_send_status = _sending_head;
	}
	if (this->_send_status == _sending_head)
		return (this->sendHead(slice));
	if (this->_send_status == _sending_body)
		return (this->sendBody(slice));
}

bool Response::read(size_t const &size)
{
	if (this->_send_status == _sending_head)
	{
		this->_header_read_index += size;
		if (this->_header_read_index >= this->_head.size())
		{
			this->_header_read_index = this->_head.size();
			this->_send_status		 = _sending_body;
		}
	}
	else if (this->_send_status == _sending_body)
	{
		size_t to_read = 0;

		this->_body_read_index += size;
		if (this->_map)
			to_read = this->_sb.st_size;
		else if (!this->_body.empty())
			to_read = this->_body.size();
		if (this->_body_read_index >= to_read)
		{
			if (this->_map)
			{
				munmap(this->_map, this->_sb.st_size);
				this->_map = NULL;
				close(this->_file_fd);
				this->_file_fd = -1;
			}
			this->_send_status = _body_sent;
			return (true);
		}
		return (false);
	}
	return (false);
}
