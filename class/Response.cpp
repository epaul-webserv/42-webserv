#include "webserv.h"
#include "Response.hpp"
#include <sys/mman.h>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>

Response::Response(int fd, Request const &request) :
	_status(_init), _conn_fd(fd), _file_fd(-1), _body(NULL),
	_header_read_index(0), _header_write_index(0), _body_read_index(0),
	_body_write_index(0),
	_content_length(0), _sent_bytes(0), _request(request)
{
	bzero(&this->_sb, sizeof(this->_sb));
}

Response::Response(Response const &Response) :
	_status(Response._status), _conn_fd(Response._conn_fd),
	_body(NULL), _header_read_index(Response._header_read_index),
	_header_write_index(Response._header_write_index),
	_body_read_index(Response._body_read_index),
	_body_write_index(Response._body_write_index),
	_content_length(Response._content_length),
	_sent_bytes(Response._sent_bytes),
	_request(Response._request)
{
	std::memcpy(this->_head, Response._head, 8192);
	this->_body = new char[sizeof(Response._body)];
	std::memcpy(this->_body, Response._body, sizeof(*Response._body));
	std::memcpy(&this->_sb, &Response._sb, sizeof(this->_sb));
}

Response::~Response(void)
{
	if (this->_file_fd != -1)
		close(this->_file_fd);
	if (this->_body)
		delete [] this->_body;
}

void Response::headAppend(std::string const &resp)
{
	const char *c_resp = resp.c_str();

	std::memcpy(this->_head + this->_header_write_index, c_resp, resp.size());
	this->_header_write_index += resp.size();
}

void Response::bodyAppend(std::string const &resp)
{
	char *new_body = new char[sizeof(this->_body) + resp.size()];

	if (this->_body)
	{
		std::memcpy(new_body, this->_body, this->_body_write_index);
		delete [] this->_body;
	}
	this->_body = new_body;
	std::memcpy(this->_body + this->_body_write_index, resp.c_str(),
				resp.size());
	this->_content_length   += resp.size();
	this->_body_write_index += resp.size();
}
