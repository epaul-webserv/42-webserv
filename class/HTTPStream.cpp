#include "HTTPStream.hpp"

HTTPStream::HTTPStream(async::Reactor &reactor, int fd, struct sockaddr_storage addr, socklen_t addr_len, Router *router) :
	router(router), addr(addr), addr_len(addr_len), io(reactor, fd), _status(_request)
{
	if (!this->router)
		throw(std::runtime_error("Cannot initialize HTTPStream with NULL router"));
}

HTTPStream::HTTPStream(HTTPStream const &HTTPStream) :
	response(HTTPStream.response), request(HTTPStream.request), router(HTTPStream.router), addr(HTTPStream.addr), addr_len(HTTPStream.addr_len), buf(HTTPStream.buf),
	io(HTTPStream.io), _status(HTTPStream._status)
{
}

HTTPStream::~HTTPStream() {}

t_request_state HTTPStream::readRequest(char *buffer, size_t len) { return (this->request.readRequest(buffer, len)); }

// bool HTTPStream::sendResponse(void) { return (this->response.sendResponse(this->_fd)); }

#include <iostream>

async::Future::Poll HTTPStream::poll() throw(async::OsError)
{
	struct iovec parts[2];
	size_t		 s;

	buf.unfilled(parts);
	container::Slice<uint8_t> buf_slice((uint8_t *)parts[0].iov_base, parts[0].iov_len);
	if (this->_status == _request)
	{
		while (io.poll_read(buf_slice, s) == Future::Ready)
		{
			buf.fill(s);
			if (this->readRequest(static_cast<char *>(parts[0].iov_base), s) == _read)
			{
				this->_status = _response_pending;
				break;
			}
		}
	}
	if (this->_status == _response_pending)
	{
		this->router->resolve(this->request, this->response);
		if (this->response.ready())
			this->_status = _response_sending;
	}
	if (this->_status == _response_sending)
	{
		container::Slice<uint8_t> slice(NULL, 0);

		this->response.sendResponse(slice);
		while (io.poll_write(slice, s) == Future::Ready)
		{
			if (this->response.read(s))
				return (Future::Ready);
		}
	}
	return (Future::Pending);
}
