#include "HTTPStream.hpp"

HTTPStream::HTTPStream(async::Reactor &reactor, int fd, struct sockaddr_storage addr, socklen_t addr_len, Router *router) :
	router(router), addr(addr), addr_len(addr_len), io(reactor, fd), buf_io(io), _status(_request), _reactor(&reactor)
{
	if (!this->router)
		throw(std::runtime_error("Cannot initialize HTTPStream with NULL router"));
}

HTTPStream::HTTPStream(HTTPStream const &HTTPStream) :
	response(HTTPStream.response), request(HTTPStream.request), router(HTTPStream.router), addr(HTTPStream.addr), addr_len(HTTPStream.addr_len), buf(HTTPStream.buf),
	io(HTTPStream.io), buf_io(io), _status(HTTPStream._status), _reactor(HTTPStream._reactor)
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
	io		 = other.io;
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
	if (this->_status == _request)
	{
		t_request_state state = this->request.readRequest(this->buf_io);
		if (state == _close)
			return (Future::Ready);
		if (state == _read)
			this->_status = _response_pending;
	}
	if (this->_status == _response_pending)
	{
		this->router->resolve(this->request, this->response, *this->_reactor, this->buf_io);
		if (this->response.ready())
		{
			std::cerr << request.getMethod() << " " << request.getURI() << " " << response.getHTTPVersion() << " - " << response.getHTTPStatus() << std::endl;
			this->_status = _response_sending;
		}
	}
	if (this->_status == _response_sending)
	{
		container::Slice<uint8_t> slice(NULL, 0);

		if (this->response.sendResponse(io))
		{
			if (this->request.getHTTPVersion() == "HTTP/1.1" && this->request.peekHeader("connection") != "close")
				return (this->keepAlive(), Future::Pending);
			return (Future::Ready);
		}
	}
	return (Future::Pending);
}

async::Future::Poll HTTPStream::keepAlive(void)
{
	this->request.clear();
	this->response.clear();
	this->_status = _request;
	return (this->poll());
}
