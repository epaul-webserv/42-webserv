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
	send_status(_first_step), file_stream(-1), header_read_index(0), header_write_index(0), body_read_index(0), body_write_index(0), content_length(0), sent_bytes(0),
	io_body(NULL)
{
	bzero(&this->sb, sizeof(this->sb));
}

Response::Response(Response const &Response) :
	send_status(Response.send_status), file_stream(-1), head(Response.head), body(Response.body), header_read_index(Response.header_read_index),
	header_write_index(Response.header_write_index), body_read_index(Response.body_read_index), body_write_index(Response.body_write_index),
	content_length(Response.content_length), sent_bytes(Response.sent_bytes), http_version(Response.http_version), http_status(Response.http_status), sb(Response.sb),
	io_body(NULL) // FIXME: copy of response generator and io body
{
	// if (Response._io_body)
	// 	this->_io_body = new async::BufIo<>(*Response._io_body);
}

Response::~Response(void)
{
	if (io_body)
		delete io_body;
}

void Response::clear()
{
	this->send_status = _first_step;
	this->file_stream.clear(-1);
	this->head.clear();
	this->body.clear();
	this->header_read_index	 = 0;
	this->header_write_index = 0;
	this->body_read_index	 = 0;
	this->body_write_index	 = 0;
	this->sent_bytes		 = 0;
	this->content_length	 = 0;
	bzero(&this->sb, sizeof(this->sb));
	if (io_body)
	{
		delete io_body;
		this->io_body = NULL;
	}
}

void Response::headAppend(std::string const &content)
{
	if (content.empty())
		return;
	this->head.append(content);
	if (*(content.end() - 1) != '\n')
		this->head.push_back('\n');
}

void Response::addHeader(std::string const &key, std::string const &value) { this->head.append(key + ": " + value + "\r\n"); }

std::string const &Response::getHTTPVersion(void) { return (this->http_version); }

void Response::setHTTPVersion(std::string const &http_version) { this->http_version = http_version; }

/// @brief Sets the response's status and sets the body if an associated file is set in the configuration or in case of error (3XX, 4XX, 5XX)
void Response::setHTTPStatus(std::string const &status, std::string const &message, std::map<std::string, std::string> const &status_pages)
{
	this->http_status = status + " " + message;
	if (status.empty())
		return;
	if (!status_pages.empty() && status_pages.find(status) != status_pages.end())
	{
		this->addHeader("Content-Type", "text/html"); // FIXME: SET THE CORRECT MIME_TYPE
		// FIXME: REPLACE BODY BY THE PAGE
	}
	else if (status[0] == '3' || status[0] == '4' || status[0] == '5')
	{
		this->bodyAppend(get_html_error(status, message));
		this->addHeader("Content-Type", "text/html");
		// FIXME: REPLACE BODY BY THE PAGE
	}
}

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

void Response::setBodyFd(int const &fd) { this->file_stream.clear(fd); }
