#include "StaticBackend.hpp"
#include <cstring>
#include <fcntl.h>
#include <strings.h>
#include <sys/mman.h>
#include <unistd.h>
#include "url_utils.hpp"
#include "webserv.h"

StaticBackend::StaticBackend(void) : _map(NULL) { bzero(&this->_sb, sizeof(this->_sb)); }

StaticBackend::StaticBackend(s_staticbackend_conf const &conf) : _map(NULL), _mime_type(conf.mime_type) { bzero(&this->_sb, sizeof(this->_sb)); }

StaticBackend::~StaticBackend(void)
{
	if (this->_map)
		munmap(this->_map, this->_sb.st_size);
}

static t_file_type fileType(std::string const &path, std::string const &root)
{
	struct stat s = {};

	if (stat((root + path).c_str(), &s) == -1)
	{
		if (errno == EACCES)
			return (_no_access);
		if (errno == ENOMEM)
			return (_err);
		return (_none);
	}
	switch (s.st_mode & S_IFMT)
	{
	case S_IFDIR:
		return (_dir);
	case S_IFREG:
		return (_file);
	default:
		return (_none);
	}
}

void StaticBackend::setContentType(std::string const &file_ext, Response &response)
{
	std::string mime_type = this->_mime_type[file_ext];

	if (this->_mime_type.empty())
		mime_type = "application/octet-stream";
	response.addHeader("Content-Type", mime_type);
}

static bool consume_body(Request &request, async::BufIo<> &client_io)
{
	char  *end;
	size_t to_read = std::strtoul(request.peekHeader("Content-Length").c_str(), &end, 10);

	if (request._body_read_index < to_read)
	{
		uint8_t					 *buf = new uint8_t[to_read - request._body_read_index + 1];
		container::Slice<uint8_t> sl(buf, to_read - request._body_read_index);
		size_t					  s = 0;
		while (client_io.poll_read(sl, s) == async::Future::Ready)
		{
			request._body_read_index += s;
			if (request._body_read_index >= to_read)
			{
				delete[] (buf);
				return (true);
			}
		}
		delete[] (buf);
		return (false);
	}
	return (true);
}

void StaticBackend::generateResponse(Request const &request, Response &response, async::Reactor &reactor, async::BufIo<> &client_io)
{
	(void)reactor;
	if (!consume_body(const_cast<Request &>(request), client_io))
		return;
	if (request.getURI().empty())
	{
		response.setHTTPVersion("HTTP/1.1");
		this->setError("400", "Bad Request", response);
	}
	else if (request.getHTTPVersion().empty())
		this->response_0_9(request, response, reactor);
	else if (request.getHTTPVersion() == "HTTP/1.0")
		this->response_1_0(request, response, reactor);
	else if (request.getHTTPVersion() == "HTTP/1.1")
		this->response_1_1(request, response, reactor);
	else
	{
		if (request.getHTTPVersion().find("HTTP/") == std::string::npos)
		{
			response.setHTTPVersion("HTTP/1.1");
			this->setError("400", "Bad Request", response);
		}
		else
		{
			response.setHTTPVersion("HTTP/1.1");
			this->setError("400", "Bad Request", response);
		}
	}
	std::cerr << request.getMethod() << " " << request.getURI() << " " << response.getHTTPVersion() << " - " << response.getHTTPStatus() << std::endl;
	response.setStatus(_ready);
}

void StaticBackend::response_0_9(Request const &request, Response &response, async::Reactor &reactor)
{
	(void)reactor;
	response.setHTTPVersion("HTTP/0.9");
	if (request.getMethod() == "GET")
	{
		switch (fileType(request.getURI(), DEFAULT_ROOT))
		{
		case _dir:
			return response.bodyAppend(get_html_dir(request.getURI(), DEFAULT_ROOT));
		case _no_access:
			return response.bodyAppend(get_html_error("405", "Forbidden"));
		case _none:
			return response.bodyAppend(get_html_error("404", "Not found"));
		case _err:
			return response.bodyAppend(get_html_error("500", "Internal Server Error"));
		case _file:
			break;
		}
		switch (this->openFile(DEFAULT_ROOT + request.getURI(), response))
		{
		case _no_permission:
			return response.bodyAppend(get_html_error("405", "Forbidden"));
		default:
			return;
		}
	}
	else
		response.headAppend(get_html_error("405", "Method not allowed"));
}

void StaticBackend::response_1_0(Request const &request, Response &response, async::Reactor &reactor)
{
	(void)reactor;
	response.setHTTPVersion("HTTP/1.0");
	if (request.getMethod() == "GET")
	{
		switch (fileType(request.getURI(), DEFAULT_ROOT))
		{
		case _dir:
			response.setHTTPStatus("200", "Continue");
			response.bodyAppend(get_html_dir(request.getURI(), DEFAULT_ROOT));
			response.addHeader("Content-Type", "text/html");
			return;
		case _no_access:
			return this->setError("403", "Forbidden", response);
		case _none:
			return this->setError("405", "Not Found", response);
		case _err:
			return this->setError("500", "Internal Server Error", response);
		case _file:
			break;
		}
		switch (this->openFile(DEFAULT_ROOT + request.getURI(), response))
		{
		case _no_permission:
			return this->setError("403", "Forbidden", response);
		default:
			response.setHTTPStatus("200", "Continue");
			this->setContentType(get_file_ext(DEFAULT_ROOT + request.getURI()), response);
			return;
		}
	}
	if (request.getMethod() == "HEAD")
	{
		switch (fileType(request.getURI(), DEFAULT_ROOT))
		{
		case _dir:
			response.setHTTPStatus("200", "Ok");
			response.addHeader("Content-Type", "text/html");
			return;
		case _no_access:
			response.setHTTPStatus("403", "Forbidden");
			response.addHeader("Content-Type", "text/html");
			return;
		case _none:
			response.setHTTPStatus("404", "Not found");
			response.addHeader("Content-Type", "text/html");
			return;
		case _err:
			response.setHTTPStatus("500", "Internal server error");
			response.addHeader("Content-Type", "text/html");
			return;
		case _file:
			response.setHTTPStatus("200", "Ok");
			this->setContentType(get_file_ext(DEFAULT_ROOT + request.getURI()), response);
		}
	}
	else
	{
		this->setError("405", "Method not allowed", response);
		return response.bodyAppend(get_html_error("405", "Method Not Allowed"));
	}
}

void StaticBackend::response_1_1(Request const &request, Response &response, async::Reactor &reactor)
{
	(void)reactor;
	response.setHTTPVersion("HTTP/1.1");
	if (request.peekHeader("connection") != "close")
		response.addHeader("Connection", "keep-alive");
	if (request.getMethod() == "GET")
	{
		switch (fileType(request.getURI(), DEFAULT_ROOT))
		{
		case _dir:
			response.setHTTPStatus("200", "Ok");
			response.bodyAppend(get_html_dir(request.getURI(), DEFAULT_ROOT));
			response.addHeader("Content-Type", "text/html");
			return;
		case _no_access:
			return this->setError("403", "Forbidden", response);
		case _none:
			return this->setError("404", "Not Found", response);
		case _err:
			return this->setError("500", "Internal Server Error", response);
		case _file:
			break;
		}
		switch (this->openFile(DEFAULT_ROOT + request.getURI(), response))
		{
		case _no_permission:
			return this->setError("403", "Forbidden", response);
		default:
			response.setHTTPStatus("200", "Ok");
			this->setContentType(get_file_ext(DEFAULT_ROOT + request.getURI()), response);
			return;
		}
	}
	if (request.getMethod() == "HEAD")
	{
		switch (fileType(request.getURI(), DEFAULT_ROOT))
		{
		case _dir:
			response.setHTTPStatus("200", "Ok");
			response.addHeader("Content-Type", "text/html");
			return;
		case _no_access:
			response.setHTTPStatus("403", "Forbidden");
			response.addHeader("Content-Type", "text/html");
			return;
		case _none:
			response.setHTTPStatus("404", "Not found");
			response.addHeader("Content-Type", "text/html");
			return;
		case _err:
			response.setHTTPStatus("500", "Internal server error");
			response.addHeader("Content-Type", "text/html");
			return;
		case _file:
			response.setHTTPStatus("200", "Ok");
			this->setContentType(get_file_ext(DEFAULT_ROOT + request.getURI()), response);
		}
	}
	else
	{
		response.setHTTPStatus("405", "Method Not Allowed");
		return response.bodyAppend(get_html_error("405", "Method Not Allowed"));
	}
}

t_file_status StaticBackend::openFile(std::string const &path, Response &response)
{
	int fd;
	if (stat((path).c_str(), &this->_sb) == -1)
		return (_not_found);
	fd = open(path.c_str(), O_RDONLY);
	if (fd < 0)
	{
		if (errno == EACCES)
			return (_no_permission);
		return (_no_permission);
	}
	std::ostringstream oss;
	oss << this->_sb.st_size;
	response.addHeader("Content-Length", oss.str());
	response.setBodyFd(fd);
	return (_open);
}
