#include "UploadBackend.hpp"
#include <cstring>
#include <fcntl.h>
#include <strings.h>
#include <sys/mman.h>
#include <unistd.h>
#include "url_utils.hpp"
#include "webserv.h"

UploadBackend::UploadBackend(std::string const &root, HTTPStream &http_stream, ARequestBody &request_body) :
	_bytes_written(0), _to_write(0), root(root), http_stream(http_stream), _response_body(NULL), _request_body(request_body), _file_buf()
{
	bzero(_file_buf, 4096);
}

UploadBackend::~UploadBackend() {}

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

t_file_status UploadBackend::openFile(std::string const &URI)
{
	if (_ofile.is_open())
		return (_open);
	_ofile.open((root + URI).c_str(), std::ios::binary);
	if (!_ofile.is_open())
		return (_no_permission);
	if (_ofile.fail())
		return (_no_permission);
	char *end;
	_to_write = std::strtol(http_stream.request.peekHeader("Content-Length").c_str(), &end, 10);
	return (_open);
}

async::Future::Poll UploadBackend::writeFile()
{
	size_t s = 0;
	while (_bytes_written < _to_write)
	{
		container::Slice<uint8_t> sl(_file_buf, std::min(_to_write - _bytes_written, static_cast<size_t>(4096)));
		if (http_stream.buf_io.poll_read(sl, s) == Pending)
			return (Pending);
		std::cerr << "fail bit : " << _ofile.fail() << std::endl;
		_ofile.write(reinterpret_cast<const std::ostream::char_type *>(_file_buf), s);
		if (_ofile.fail())
		{
			std::cerr << "Write failed. System error: " << std::strerror(errno) << std::endl;
			http_stream.setHTTPStatus("500", "Internal Server Error");
			return (Ready);
		}
		_bytes_written += s;
	}
	return (Ready);
}

async::Future::Poll UploadBackend::setResponse() throw(async::OsError)
{
	if (http_stream.request.getMethod() == "POST")
	{
		t_file_status status = openFile(http_stream.request.getURI());
		switch (status)
		{
		case _no_permission:
			http_stream.setHTTPStatus("403", "Forbidden");
			break;
		case _open:
			if (writeFile() == Pending)
				return (Pending);
			http_stream.response.addHeader("Content-Length", "0");
			http_stream.setHTTPStatus("200", "Created");
			break;
		default:
			http_stream.setHTTPStatus("500", "Internal Server Error");
		}
	}
	else
		return http_stream.setHTTPStatus("405", "Method Not Allowed"), Ready;
	return Ready;
}

async::Future::Poll UploadBackend::poll() throw(async::OsError)
{
	if (this->setResponse() == Pending)
		return (Pending);
	if (http_stream.poll_send_headers(_response_body) == Pending)
		return (Pending);
	return (Ready);
}
