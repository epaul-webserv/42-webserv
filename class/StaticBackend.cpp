#include "StaticBackend.hpp"
#include <cstring>
#include <fcntl.h>
#include <strings.h>
#include <sys/mman.h>
#include <unistd.h>
#include "webserv.h"

StaticBackend::StaticBackend(void) : _map(NULL)
{
	bzero(&this->_sb, sizeof(this->_sb));
}

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

void StaticBackend::generateResponse(Request const &request, Response &response)
{
	if (request.getURI().empty())
	{
		response.setHTTPVersion("HTTP/1.1");
		this->setError("400", "Bad Request", response);
	}
	else if (request.getHTTPVersion().empty())
		this->response_0_9(request, response);
	else if (request.getHTTPVersion() == "HTTP/1.0")
		this->response_1_0(request, response);
	else if (request.getHTTPVersion() == "HTTP/1.1")
		this->response_1_1(request, response);
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
	std::cerr << request.getMethod() << " " << request.getURI() << " "
			  << response.getHTTPVersion() << " - " << response.getHTTPStatus()
			  << std::endl;
	response.setStatus(_ready);
}

void StaticBackend::response_0_9(Request const &request, Response &response)
{
	response.setHTTPVersion("HTTP/0.9");
	if (request.getMethod() == "GET")
	{
		switch (fileType(request.getURI(), DEFAULT_ROOT))
		{
		case _dir:
			return response.bodyAppend(
				get_html_dir(request.getURI(), DEFAULT_ROOT));
		case _no_access:
			return response.bodyAppend(get_html_error("405", "Forbidden"));
		case _none:
			return response.bodyAppend(get_html_error("404", "Not found"));
		case _err:
			return response.bodyAppend(
				get_html_error("500", "Internal Server Error"));
		case _file:
			break;
		}
		switch (response.openFile(DEFAULT_ROOT + request.getURI()))
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

void StaticBackend::response_1_0(Request const &request, Response &response)
{
	response.setHTTPVersion("HTTP/1.0");
	if (request.getMethod() == "GET")
	{
		switch (fileType(request.getURI(), DEFAULT_ROOT))
		{
		case _dir:
			response.setHTTPStatus("200", "Continue");
			response.bodyAppend(get_html_dir(request.getURI(), DEFAULT_ROOT));
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
		switch (response.openFile(DEFAULT_ROOT + request.getURI()))
		{
		case _no_permission:
			return this->setError("403", "Forbidden", response);
		default:
			response.setHTTPStatus("200", "Continue");
			return;
		}
	}
	// if (request.getMethod() == "HEAD")
	//     return (this->HEAD());
	// if (request.getMethod() == "POST")
	//     return (this->POST());
	else
	{
		this->setError("405", "Method not allowed", response);
		return response.bodyAppend(get_html_error("405", "Method Not Allowed"));
	}
}

void StaticBackend::response_1_1(Request const &request, Response &response)
{
	response.setHTTPVersion("HTTP/1.1");
	if (request.getMethod() == "GET")
	{
		switch (fileType(request.getURI(), DEFAULT_ROOT))
		{
		case _dir:
			response.setHTTPStatus("200", "Ok");
			response.bodyAppend(get_html_dir(request.getURI(), DEFAULT_ROOT));
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
		switch (response.openFile(DEFAULT_ROOT + request.getURI()))
		{
		case _no_permission:
			return this->setError("403", "Forbidden", response);
		default:
			response.setHTTPStatus("200", "Ok");
			return;
		}
	}
	// if (request.getMethod() == "HEAD")
	//     return (this->HEAD());
	// if (request.getMethod() == "POST")
	//     return (this->POST());
	// if (request.getMethod() == "OPTIONS")
	//     return (this->OPTIONS());
	// if (request.getMethod() == "PUT")
	//     return (this->PUT());
	// if (request.getMethod() == "DELETE")
	//     return (this->DELETE());
	// if (request.getMethod() == "TRACE")
	//     return (this->TRACE());
	// if (request.getMethod() == "CONNECT")
	//     return (this->CONNECT());
	else
	{
		response.headAppend(request.getHTTPVersion() +
							" 405 Method Not Allowed\r\n");
		return response.bodyAppend(get_html_error("405", "Method Not Allowed"));
	}
}
