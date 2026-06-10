#include "StaticBackend.hpp"
#include <cstring>
#include <fcntl.h>
#include <strings.h>
#include <sys/mman.h>
#include <unistd.h>
#include "MimeType.hpp"
#include "url_utils.hpp"
#include "webserv.h"

StaticBackend::StaticBackend(void) : _map(NULL) { bzero(&this->_sb, sizeof(this->_sb)); }

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
	std::string mime_type = MimeType::mime_type[file_ext];

	if (MimeType::mime_type.empty())
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
	response.setStatus(_ready);
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
			return response.setError("403", "Forbidden");
		case _none:
			return response.setError("404", "Not Found");
		case _err:
			return response.setError("500", "Internal Server Error");
		case _file:
			break;
		}
		switch (this->openFile(DEFAULT_ROOT + request.getURI(), response))
		{
		case _no_permission:
			return response.setError("403", "Forbidden");
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
