#include "CGIResponseGenerator.hpp"

CGIResponseGenerator::CGIResponseGenerator(Request const &request, Response &response, async::Reactor &reactor, async::BufIo<> &bodyIo, int cgi_in_fd,
										   int cgi_out_fd) :
	_request(request), _response(response), _bodyIo(bodyIo), _cgi_in(reactor, cgi_in_fd), _cgi_out(NULL), _request_body_read_done(false), _body_write_done(false),
	_response_headers_read_done(false), _request_body_read_len(0), _request_body_read_index(0), _response_body_read_len(0), _response_body_read_index(0)
{
	_response.setHTTPVersion(_request.getHTTPVersion());
	std::string content_len = _request.peekHeader("content-length");
	if (content_len == "")
	{
		_body_len				= 0;
		_request_body_read_done = true;
		_body_write_done		= true;
	}
	else
	{
		char *ptr;
		_body_len = std::strtoul(content_len.c_str(), &ptr, 10);
	}
	_cgi_out = new async::BufIo<>(reactor, cgi_out_fd);
}

async::Future::Poll CGIResponseGenerator::poll() throw(async::OsError)
{
	if (!_request_body_read_done && _request_body_read_index == _request_body_read_len)
	{
		container::Slice<uint8_t> slice(_request_body_buf, 4096);
		if (_bodyIo.poll_read(slice, _request_body_read_len) == Ready)
		{
			_body_len -= _request_body_read_len;
			if (_request_body_read_len == 0 || _body_len <= 0)
				_request_body_read_done = true;
			else
				_request_body_read_index = 0;
		}
	}
	if (!_body_write_done && _request_body_read_index < _request_body_read_len)
	{
		container::Slice<uint8_t> slice(_request_body_buf + _request_body_read_index, _request_body_read_len - _request_body_read_index);
		size_t					  written;
		if (_cgi_in.poll_write(slice, written) == Ready)
		{
			_request_body_read_index += written;
			if (_request_body_read_done && _request_body_read_index == _request_body_read_len)
				_body_write_done = true;
		}
	}
	// This should only read headers
	if (!_response_headers_read_done)
	{
		std::vector<uint8_t> line_buf;
		if (_cgi_out->poll_read_line(line_buf) == Pending)
			return Pending;
		if (line_buf.size() == 0 || (line_buf.size() == 2 && line_buf[0] == '\r'))
			_response_headers_read_done = true;
		else
		{
			if (_request.getHTTPVersion() != "")
			{
				std::string key, value;
				if (parse_cgi_header(line_buf, key, value))
				{
				}
				else
				{
				}
			}
			_response.headAppend(std::string(line_buf.begin(), line_buf.end()));
			// cgi_body.append(reinterpret_cast<const char *>(_response_body_buf), _response_body_read_len);
		}
	}
	if (_request_body_read_done && _body_write_done && _response_headers_read_done)
	{
		_response.setHTTPStatus("200", "Ok");
		return Ready;
	}

	return Pending;
}

#include <string>
#include <vector>

bool CGIResponseGenerator::parse_cgi_header(const std::vector<uint8_t> &raw_line, std::string &key, std::string &value)
{
	if (raw_line.empty())
		return false;

	std::string line(reinterpret_cast<const char *>(&raw_line[0]), raw_line.size());

	while (!line.empty() && (line.at(line.size() - 1) == '\n' || line.at(line.size() - 1) == '\r'))
		line.erase(line.size() - 1);

	if (line.empty())
		return false;

	size_t colon_pos = line.find(':');
	if (colon_pos == std::string::npos || colon_pos == 0)
		return false;

	size_t key_start = 0;
	size_t key_end	 = colon_pos;

	while (key_end > key_start && (line[key_end - 1] == ' ' || line[key_end - 1] == '\t'))
	{
		--key_end;
	}

	if (key_start == key_end)
		return false;

	size_t val_start = colon_pos + 1;
	size_t val_end	 = line.size();

	while (val_start < val_end && (line[val_start] == ' ' || line[val_start] == '\t'))
		++val_start;
	while (val_end > val_start && (line[val_end - 1] == ' ' || line[val_end - 1] == '\t'))
		--val_end;
	key	  = line.substr(key_start, key_end - key_start);
	value = line.substr(val_start, val_end - val_start);

	return true;
}

void CGIResponseGenerator::parse_cgi_status_value(const std::string &status_value, std::string &status_code, std::string &status_message)
{
	status_code.clear();
	status_message.clear();

	std::string::size_type space_pos = status_value.find(' ');

	if (space_pos == std::string::npos)
		status_code = status_value;
	else
	{
		status_code						 = status_value.substr(0, space_pos);
		std::string::size_type msg_start = space_pos;
		while (msg_start < status_value.size() && (status_value[msg_start] == ' ' || status_value[msg_start] == '\t'))
			++msg_start;

		if (msg_start < status_value.size())
			status_message = status_value.substr(msg_start);
	}
}

CGIResponseGenerator::~CGIResponseGenerator() {}
