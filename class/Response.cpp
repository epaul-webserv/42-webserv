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
	_send_status(_first_step), _fetch_status(_init), _file_stream(-1), _header_read_index(0), _header_write_index(0), _body_read_index(0), _body_write_index(0),
	_content_length(0), _sent_bytes(0), _response_generator(NULL), _io_body(NULL)
{
	bzero(&this->_sb, sizeof(this->_sb));
}

Response::Response(Response const &Response) :
	_send_status(Response._send_status), _fetch_status(Response._fetch_status), _file_stream(-1), _head(Response._head), _body(Response._body),
	_header_read_index(Response._header_read_index), _header_write_index(Response._header_write_index), _body_read_index(Response._body_read_index),
	_body_write_index(Response._body_write_index), _content_length(Response._content_length), _sent_bytes(Response._sent_bytes), _http_version(Response._http_version),
	_http_status(Response._http_status), _sb(Response._sb), _response_generator(NULL)
{
	// if (Response._io_body)
	// 	this->_io_body = new async::BufIo<>(*Response._io_body);
}

Response::~Response(void)
{
	// if (this->_map)
	// 	munmap(this->_map, this->_sb.st_size);
	if (_response_generator)
		delete _response_generator;
	if (_io_body)
		delete _io_body;
}

void Response::clear()
{
	// if (this->_map)
	// 	munmap(this->_map, this->_sb.st_size);
	this->_send_status = _first_step;
	this->_file_stream.clear(-1);
	this->_fetch_status = _init;
	// this->_map			= NULL;
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
	if (_io_body)
	{
		delete _io_body;
		this->_io_body = NULL;
	}
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

void Response::setGenerator(Request const &request, Response &response, async::Reactor &reactor, async::BufIo<> &bodyIo, int cgi_in_fd, int cgi_out_fd)
{
	_response_generator = new CGIResponseGenerator(request, response, reactor, bodyIo, cgi_in_fd, cgi_out_fd);
}

void Response::setBodyFd(int const &fd) { this->_file_stream.clear(fd); }

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
	if (this->_content_length)
		head << "Content-Length: " << this->_content_length << "\r\n";
	this->_head.insert(0, head.str());
	this->_head.append("\r\n");
}

bool Response::sendHead(async::Io &client_io)
{
	if (this->_send_buf.size() - this->_header_read_index > 0)
	{
		size_t s;
		while (client_io.poll_write(container::Slice<uint8_t>(&this->_send_buf[this->_header_read_index], this->_send_buf.size() - this->_header_read_index), s) ==
			   async::Future::Ready)
		{
			this->_header_read_index += s;
			if (this->_header_read_index >= this->_send_buf.size())
			{
				this->_send_status = _sending_body;
				return (this->sendBody(client_io));
			}
		}
	}
	if (this->_header_read_index >= this->_send_buf.size())
	{
		this->_send_status = _sending_body;
		return (this->sendBody(client_io));
	}
	return (false);
}

bool Response::sendBody(async::Io &client_io)
{
	if (this->_io_body)
		async::CopyBufIoToIo<4096> test(*this->_io_body, client_io);
	else if (this->_file_stream.is_open())
	{
		std::vector<uint8_t> buf = this->_file_stream.peek_str(4096);
		size_t				 s;

		if (client_io.poll_write(container::Slice<uint8_t>(&buf[0], buf.size()), s) == async::Future::Ready)
		{
			this->_file_stream.consume(s);
			if (this->_file_stream.end())
				return (true);
		}
		return (false);
	}
	return (true);
}

bool Response::sendResponse(async::Io &client_io)
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
		return (this->sendHead(client_io));
	if (this->_send_status == _sending_body)
		return (this->sendBody(client_io));
	return (true);
}
