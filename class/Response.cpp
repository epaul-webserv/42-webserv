#include "Response.hpp"
#include <async>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <strings.h>
#include <sys/mman.h>
#include <unistd.h>
#include "webserv.h"

Response::Response(void) :
	send_status(_first_step), header_read_index(0), header_write_index(0), body_read_index(0), body_write_index(0), content_length(0), sent_bytes(0)
{
	bzero(&this->sb, sizeof(this->sb));
}

Response::Response(Response const &Response) :
	send_status(Response.send_status), head(Response.head), body(Response.body), header_read_index(Response.header_read_index),
	header_write_index(Response.header_write_index), body_read_index(Response.body_read_index), body_write_index(Response.body_write_index),
	content_length(Response.content_length), sent_bytes(Response.sent_bytes), http_version(Response.http_version), http_status(Response.http_status),
	sb(Response.sb) // FIXME: copy of response generator and io body
{
	// if (Response._io_body)
	// 	this->_io_body = new async::BufIo<>(*Response._io_body);
}

Response::~Response(void) {}

void Response::clear()
{
	this->send_status = _first_step;
	this->head.clear();
	this->body.clear();
	this->header_read_index	 = 0;
	this->header_write_index = 0;
	this->body_read_index	 = 0;
	this->body_write_index	 = 0;
	this->sent_bytes		 = 0;
	this->content_length	 = 0;
	bzero(&this->sb, sizeof(this->sb));
}

void Response::headAppend(std::string const &content)
{
	if (content.empty())
		return;
	this->head.append(content);
	if (*(content.end() - 1) != '\n')
		this->head.push_back('\n');
}

void Response::addHeader(std::string const &key, std::string const &value) { this->headers[key] = value; }

std::string const &Response::getHTTPVersion(void) { return (this->http_version); }

void Response::setHTTPVersion(std::string const &http_version) { this->http_version = http_version; }

void Response::bodyAppend(std::string const &content)
{
	this->body.append(content);
	this->content_length += content.size();
}

void Response::setHttpVersion(std::string const &http_version)
{
	if (http_version != "HTTP/1.0" && http_version != "HTTP/1.1")
	{
		throw(std::runtime_error("Invalid HTTP version"));
	}
	this->http_version = http_version;
}

std::string const &Response::getHTTPStatus(void) { return (this->http_status); }
