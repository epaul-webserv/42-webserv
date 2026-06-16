#include "StaticBackend.hpp"
#include <fcntl.h>
#include <strings.h>
#include "url_utils.hpp"
#include "webserv.h"

StaticBackend::StaticBackend(std::string const &root, HTTPStream &http_stream) :
	_sb(), _file_ok(false), _response_set(false), root(root), http_stream(http_stream), _response_body(NULL), _file_stream(-1)
{
	bzero(&this->_sb, sizeof(this->_sb));
}

StaticBackend::~StaticBackend() {}

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

	if (mime_type.empty())
		mime_type = "application/octet-stream";
	response.addHeader("Content-Type", mime_type);
}

static bool consume_body(Request &request, async::BufIo<> &client_io)
{
	char		*end;
	const size_t to_read = std::strtoul(request.peekHeader("Content-Length").c_str(), &end, 10);

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

async::Future::Poll StaticBackend::setResponse() throw(async::OsError)
{
	// if (!consume_body(const_cast<Request &>(request), client_io))
	// 	return; FIXME: skip body
	if (http_stream.request.getMethod() == "GET")
	{
		switch (fileType(http_stream.request.getURI(), DEFAULT_ROOT))
		{
		case _dir:
			http_stream.response.bodyAppend(get_html_dir(http_stream.request.getURI(), DEFAULT_ROOT));
			http_stream.response.addHeader("Content-Type", "text/html");
			http_stream.setHTTPStatus("200", "Ok");
			return Ready;
		case _no_access:
			return http_stream.setHTTPStatus("403", "Forbidden"), Ready;
		case _none:
			return http_stream.setHTTPStatus("404", "Not Found"), Ready;
		case _err:
			return http_stream.setHTTPStatus("500", "Internal Server Error"), Ready;
		case _file:
			break;
		}
		switch (this->openFile(DEFAULT_ROOT + http_stream.request.getURI(), http_stream.response))
		{
		case _no_permission:
			return http_stream.setHTTPStatus("403", "Forbidden"), Ready;
		default:
			setContentType(get_file_ext(DEFAULT_ROOT + http_stream.request.getURI()), http_stream.response);
			http_stream.setHTTPStatus("200", "Ok");
			this->_file_ok = true;
			return Ready;
		}
	}
	if (http_stream.request.getMethod() == "HEAD")
	{
		switch (fileType(http_stream.request.getURI(), DEFAULT_ROOT))
		{
		case _dir:
			http_stream.response.addHeader("Content-Type", "text/html");
			http_stream.setHTTPStatus("200", "Ok");
			return Ready;
		case _no_access:
			http_stream.setHTTPStatus("403", "Forbidden");
			return Ready;
		case _none:
		case _err:
			http_stream.setHTTPStatus("404", "Not found");
			return Ready;
		case _file:
			setContentType(get_file_ext(DEFAULT_ROOT + http_stream.request.getURI()), http_stream.response);
			http_stream.setHTTPStatus("200", "Ok");
		}
	}
	else
		return http_stream.setHTTPStatus("405", "Method Not Allowed"), Ready;
	return Ready;
}

async::Future::Poll StaticBackend::setBodyWriter() throw(async::OsError)
{
	if (_response_body == NULL)
	{
		if (this->_file_ok && http_stream.poll_send_headers(_sb.st_size, _response_body) == Pending)
			return Pending;
		if (!this->_file_ok && http_stream.poll_send_headers(0, _response_body) == Pending)
			return Pending;
	}
	return Ready;
}

async::Future::Poll StaticBackend::sendResponse() throw(async::OsError)
{
	if (this->_file_stream.is_open())
	{
		std::vector<uint8_t> buf = this->_file_stream.peek_str(4096);
		size_t				 s;

		while (!this->_file_stream.end())
		{
			buf = this->_file_stream.peek_str(4096);
			if (_response_body->poll_write(container::Slice<uint8_t>(&buf[0], buf.size()), s) == async::Future::Pending)
				return (Pending);
			this->_file_stream.consume(s);
		}
		if (this->_response_body->poll_flush() == Pending)
			return (Pending);
	}
	return (Ready);
}

async::Future::Poll StaticBackend::poll() throw(async::OsError)
{
	if (!_response_set && this->setResponse() == Pending) // ATTENTION AU RERENTRAGE
		return (Pending);
	_response_set = true;
	if (this->setBodyWriter() == Pending)
		return (Pending);
	if (this->_response_body && this->sendResponse() == Pending)
		return (Pending);
	return (Ready);
}

std::string StaticBackend::generateEtag() const
{
	std::ostringstream oss;
	oss << "\"" << _sb.st_mtime << _sb.st_size << "\"";
	return (oss.str());
}

bool StaticBackend::conditionalGet(std::string const &etag)
{
	std::string client_etag = http_stream.request.peekHeader("if-none-match");
	if (!client_etag.empty())
	{
		if (client_etag == etag)
			return (false);
	}
	std::string mod_cond = http_stream.request.peekHeader("if-modified-since");
	if (!mod_cond.empty())
	{
		try
		{
			Date client_date(mod_cond);
			if (Date(&_sb.st_mtime) < Date(mod_cond))
				return (false);
		}
		catch (Date::InvalidFormatException &e)
		{
			(void)e;
		}
	}
	return (true);
}

t_file_status StaticBackend::openFile(std::string const &path, Response &response)
{
	int fd;
	if (stat((path).c_str(), &this->_sb) == -1)
		return (_not_found);
	Date mod_date(&_sb.st_mtime);
	if (mod_date <= Date())
		response.addHeader("Last-Modified", mod_date.str());
	else
		response.addHeader("Last-Modified", Date().str());
	std::string etag = generateEtag();
	if (!conditionalGet(etag))
		return (_not_modified);
	_ifile.open(path.c_str(), std::ios::binary);
	if (!_ifile.is_open())
		return (_no_permission);
	}
	std::ostringstream oss;
	oss << this->_sb.st_size;
	response.addHeader("Content-Length", oss.str());
	_file_stream.clear(fd);
	return (_open);
}
