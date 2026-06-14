#include "HTTPStream.hpp"
#include "webserv.h"

HTTPStream::HTTPStream(async::Reactor &reactor, int fd, struct sockaddr_storage addr, socklen_t addr_len, Router *router) :
	router(router), addr(addr), addr_len(addr_len), buf_io(reactor, fd), _status(_fetching_request), _reactor(&reactor), _backend(NULL),
	_headers_status(_initializing_headers), _headers_sent_bytes(0)
{
	if (!this->router)
		throw(std::runtime_error("Cannot initialize HTTPStream with NULL router"));
}

HTTPStream::HTTPStream(HTTPStream const &other) :
	response(other.response), request(other.request), router(other.router), addr(other.addr), addr_len(other.addr_len), buf(other.buf), buf_io(other.buf_io),
	_status(other._status), _reactor(other._reactor), _backend(NULL), _headers_status(other._headers_status), _headers_sent_bytes(other._headers_sent_bytes)
{
}

HTTPStream::~HTTPStream() { delete this->_backend; }

HTTPStream &HTTPStream::operator=(HTTPStream const &other)
{
	if (this != &other)
	{
		response = other.response;
		request	 = other.request;
		router	 = other.router;
		addr	 = other.addr;
		addr_len = other.addr_len;
		buf		 = other.buf;
		buf_io	 = other.buf_io;
		_status	 = other._status;
		_reactor = other._reactor;
	}
	return (*this);
}

// t_request_state HTTPStream::readRequest(char *buffer, size_t len) { return (this->request.readRequest(buffer, len)); }

async::Future::Poll HTTPStream::poll() throw(async::OsError)
{
	struct iovec parts[2];

	buf.unfilled(parts);
	container::Slice<uint8_t> buf_slice((uint8_t *)parts[0].iov_base, parts[0].iov_len);
	if (this->_status == _fetching_request)
	{

		t_request_state state = this->request.readRequest(this->buf_io);
		if (state == _reading)
			return (Future::Pending);
		if (state == _close)
			return (Future::Ready);
		if (state == _read)
			this->_status = _getting_backend;
	}
	if (this->_status == _getting_backend)
	{
		_backend	  = this->router->resolve(this->request, this->response, *this->_reactor, *this);
		this->_status = _processing_response;
	}
	if (this->_status == _processing_response)
	{
		container::Slice<uint8_t> slice(NULL, 0);

		if (_backend == NULL)
		{
			if (this->sendHeaders() == Pending)
				return Pending;
		}
		if (_backend && _backend->poll() == Pending)
			return (Pending);
		if (this->request.getHTTPVersion() == "HTTP/1.1" && this->request.peekHeader("connection") != "close")
			return (this->keepAlive(), Future::Pending);
		return (Future::Ready);
	}
	return (Future::Ready);
}

async::Future::Poll HTTPStream::keepAlive(void)
{
	this->request.clear();
	this->response.clear();
	this->status_pages.clear();
	this->_status = _fetching_request;
	delete this->_backend;
	this->_backend			  = NULL;
	this->_headers_status	  = _initializing_headers;
	this->_headers_sent_bytes = 0;
	return (this->poll());
}

void HTTPStream::headersAddBase()
{
	std::ostringstream head;

	if (request.http_version == "HTTP/0.9")
		return;
	head << request.http_version + " " + response.http_status << "\r\n";
	head << "Server: " << PROGNAME << "\r\n";
	head << "Date: " << Date().str() << "\r\n";
	if (response.content_length)
	{
		std::ostringstream tmp;
		tmp << response.content_length;
		response.headers["Content-Length"] = tmp.str();
	}
	std::map<std::string, std::string>::const_iterator ite = response.headers.end();
	for (std::map<std::string, std::string>::const_iterator it = response.headers.begin(); it != ite; ++it)
		head << it->first << ": " << it->second << "\r\n";
	response.head.insert(0, head.str());
	response.head.append("\r\n");
}

/// @brief Sets the response's status and sets the body if an associated file is set in the configuration or in case of error (3XX, 4XX, 5XX)
void HTTPStream::setHTTPStatus(std::string const &status, std::string const &message)
{
	response.http_status = status + " " + message;
	if (status.empty())
		return;
	if (!status_pages.empty() && status_pages.find(status) != status_pages.end())
	{
		response.addHeader("Content-Type", "text/html"); // FIXME: SET THE CORRECT MIME_TYPE
		// FIXME: REPLACE BODY BY THE PAGE
	}
	else if (status[0] == '3' || status[0] == '4' || status[0] == '5')
	{
		response.bodyAppend(get_html_error(status, message));
		response.addHeader("Content-Type", "text/html"); // FIXME: COULD ADD DUPLICATE HEADER
	}
}

async::Future::Poll HTTPStream::sendHeaders() throw(async::OsError)
{
	if (this->_headers_status == _initializing_headers)
	{
		this->headersAddBase();
		if (!request.http_version.empty())
			response.send_buf = std::vector<uint8_t>(response.head.c_str(), response.head.c_str() + response.head.size());
		response.send_buf.insert(response.send_buf.end(), response.body.begin(), response.body.end());
		this->_headers_status = _sending_headers;
	}
	if (this->_headers_status == _sending_headers)
	{
		while (_headers_sent_bytes < response.send_buf.size())
		{
			size_t s;
			if (this->buf_io.poll_write(container::Slice<uint8_t>(&response.send_buf[_headers_sent_bytes], response.send_buf.size() - _headers_sent_bytes), s) ==
				async::Future::Pending)
				return (Pending);
			_headers_sent_bytes += s;
		}
		if (this->buf_io.poll_flush() == Pending)
			return Pending;
	}
	return (Ready);
}
