#include "CGIResponseGenerator.hpp"

CGIResponseGenerator::CGIResponseGenerator(Request const &request, Response &response, async::Reactor &reactor, async::Io &bodyIo, int cgi_in_fd, int cgi_out_fd) :
	_request(request), _response(response), _bodyIo(bodyIo), _cgi_in(reactor, cgi_in_fd), _cgi_out(reactor, cgi_out_fd), _request_body_read_done(false),
	_body_write_done(false), _response_body_read_done(false), _request_body_read_len(0), _request_body_read_index(0), _response_body_read_len(0),
	_response_body_read_index(0)
{
}

async::Future::Poll CGIResponseGenerator::poll() throw(async::OsError)
{
	_request_body_read_done = true;
	_body_write_done		= true;
	// if (!_request_body_read_done && _request_body_read_index == _request_body_read_len)
	// {
	// 	container::Slice<uint8_t> slice(_request_body_buf, 4096);
	// 	if (_bodyIo.poll_read(slice, _request_body_read_len) == Ready)
	// 	{

	// 		if (_request_body_read_len == 0)
	// 			_request_body_read_done = true;
	// 		else
	// 			_request_body_read_index = 0;
	// 	}
	// }
	// if (!_body_write_done && _request_body_read_index < _request_body_read_len)
	// {
	// 	container::Slice<uint8_t> slice(_request_body_buf + _request_body_read_index, _request_body_read_len - _request_body_read_index);
	// 	size_t					  written;
	// 	if (_bodyIo.poll_write(slice, written) == Ready)
	// 	{
	// 		_request_body_read_index += written;
	// 		if (_request_body_read_done && _request_body_read_index == _request_body_read_len)
	// 			_body_write_done = true;
	// 	}
	// }
	// This should only read headers
	if (!_response_body_read_done)
	{
		container::Slice<uint8_t> slice(_response_body_buf, 4096);
		if (_cgi_out.poll_read(slice, _response_body_read_len) == Ready)
		{
			if (_response_body_read_len == 0)
				_response_body_read_done = true;
			else
				cgi_body.append(reinterpret_cast<const char *>(_response_body_buf), _response_body_read_len);
		}
	}
	if (_request_body_read_done && _body_write_done && _response_body_read_done)
	{
		size_t		pos	 = cgi_body.find(std::string("\r\n\r\n"));
		std::string head = cgi_body.substr(0, pos);
		std::string body = cgi_body.substr(pos + 4, cgi_body.length());
		std::cout << "Headers\n" << head << std::endl;
		std::cout << "Body\n" << body << std::endl;
		_response.headAppend(head);
		_response.bodyAppend(body);
		_response.setHTTPVersion("HTTP/1.1");
		_response.setHTTPStatus("200", "Ok");
		return Ready;
	}

	return Pending;
}

CGIResponseGenerator::~CGIResponseGenerator() {}
