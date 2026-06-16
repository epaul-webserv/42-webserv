#include "Response.hpp"
#include <fcntl.h>
#include <strings.h>
#include "webserv.h"

Response::Response() : send_status(_first_step), content_length(0), sent_bytes(0), sb() { bzero(&this->sb, sizeof(this->sb)); }

Response::Response(Response const &Response) :
	send_status(Response.send_status), head(Response.head), body(Response.body), content_length(Response.content_length), sent_bytes(Response.sent_bytes),
	http_version(Response.http_version), http_status(Response.http_status), sb(Response.sb)
{
}

Response::~Response() {}

void Response::clear()
{
	this->send_status = _first_step;
	this->head.clear();
	this->body.clear();
	this->headers.clear();
	this->content_length = 0;
	this->sent_bytes	 = 0;
	this->http_version.clear();
	this->http_status.clear();
	this->send_buf.clear();
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

std::string const &Response::getHTTPVersion() const { return (this->http_version); }

void Response::setHTTPVersion(std::string const &new_http_version) { this->http_version = new_http_version; }

void Response::bodyAppend(std::string const &content)
{
	this->body.append(content);
	this->content_length += content.size();
}

std::string const &Response::getHTTPStatus() const { return (this->http_status); }
