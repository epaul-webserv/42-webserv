#include "HTTPStream.hpp"

HTTPStream::HTTPStream(void) : _fd(-1) {}

HTTPStream::HTTPStream(int fd) : _fd(fd) {}

HTTPStream::~HTTPStream() {}

void HTTPStream::clear(int fd)
{
	this->_fd = fd;
	request.clear();
	response.clear();
}

t_request_state HTTPStream::readRequest(void) { return (this->request.readRequest(this->_fd)); }

bool HTTPStream::sendResponse(void) { return (this->response.sendResponse(this->_fd)); }
