#include "HTTPStream.hpp"
#include "webserv.h"

HTTPStream::HTTPStream(async::Reactor &reactor, int fd, struct sockaddr_storage addr, socklen_t addr_len, Router *router) :
	router(router), addr(addr), addr_len(addr_len), buf_io(reactor, fd), _status(_fetching_request), _reactor(&reactor), _headers_status(_initializing_headers),
	_headers_sent_bytes(0)
{
	if (!this->router)
		throw(std::runtime_error("Cannot initialize HTTPStream with NULL router"));
}

HTTPStream::HTTPStream(HTTPStream const &other) :
	response(other.response), request(other.request), router(other.router), addr(other.addr), addr_len(other.addr_len), buf(other.buf), buf_io(other.buf_io),
	_status(other._status), _reactor(other._reactor), _headers_status(other._headers_status), _headers_sent_bytes(other._headers_sent_bytes)
{
}

HTTPStream::~HTTPStream() {}

HTTPStream &HTTPStream::operator=(HTTPStream const &other)
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
		if (state == _close)
			return (Future::Ready);
		if (state == _read)
			this->_status = _getting_backend;
	}
	if (this->_status == _getting_backend)
	{
		_backend = this->router->resolve(this->request, this->response, *this->_reactor, this->buf_io);
		if (_backend == NULL)
			this->_status = _sending_response;
		this->_status = _processing_response;
	}
	if (this->_status == _processing_response)
	{
		container::Slice<uint8_t> slice(NULL, 0);

		if (_backend->poll() == Ready)
		{
			if (this->request.getHTTPVersion() == "HTTP/1.1" && this->request.peekHeader("connection") != "close")
				return (this->keepAlive(), Future::Pending);
			return (Future::Ready);
		}
	}
	if (this->_status == _sending_response)
	{
		container::Slice<uint8_t> slice(NULL, 0);

		// if (this->response.sendResponse(buf_io)) FIXME: Implement new api
		// {
		// 	if (this->request.getHTTPVersion() == "HTTP/1.1" && this->request.peekHeader("connection") != "close")
		// 		return (this->keepAlive(), Future::Pending);
		// 	return (Future::Ready);
		// }
	}
	return (Future::Pending);
}

async::Future::Poll HTTPStream::keepAlive(void)
{
	this->request.clear();
	this->response.clear();
	this->_status = _fetching_request;
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
		head << "Content-Length: " << response.content_length << "\r\n";
	response.head.insert(0, head.str());
	response.head.append("\r\n");
}
