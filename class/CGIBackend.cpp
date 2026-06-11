#include "CGIBackend.hpp"
#include <iostream>
#include <string>
#include <vector>
#include "url_utils.hpp"
#include "webserv.h"

CGIBackend::CGIBackend(std::vector<std::string> const &args, Request const &request, std::string const &root, Response &response, async::Reactor &reactor,
					   async::BufIo<> &io, std::map<std::string, std::string> const &status_pages) :
	ABackend(request, root, response, reactor, io, status_pages), _args(args), _cgi_io(NULL), _request_body_read_done(false), _body_write_done(false),
	_response_headers_read_done(false), _status_set(false), _status(_init)
{
}


CGIBackend::~CGIBackend(void) {}

async::Future::Poll CGIBackend::poll() throw(async::OsError)
{
	if (_status == _init)
		init();
	if (_status == _pending)
		return process_io();
}

// The request has been routed towards the cgi, we start the program and setup
// the io to process it
void CGIBackend::init()
{
	std::string				 encoding = _request.peekHeader("transfer-encoding");
	std::vector<std::string> env_vector;

	env_vector.push_back("QUERY_STRING=" + _request.getQueryString());
	env_vector.push_back("REQUEST_METHOD=" + _request.getMethod());

	// The name of the file being executed
	std::string script_name		 = get_file_name(_request.getURI());
	std::string script_full_path = std::string(DEFAULT_ROOT) + "/" + script_name;

	// This is supposed to be whats after the script filename, and before the query string,
	// but Logan Martin told me to trust him, "tkt" was his explanation, it convinced me
	env_vector.push_back("PATH_INFO=" + script_full_path);
	// Internal decoded URI
	env_vector.push_back("PATH_TRANSLATED=" + script_full_path);
	// Tells the cgi EXACTLY what file to run
	env_vector.push_back("SCRIPT_FILENAME=" + script_full_path);

	env_vector.push_back("SERVER_PORT=" + DEFAULT_PORT);

	// name of teh server on the network ?
	env_vector.push_back("SERVER_NAME=" + std::string(PROGNAME));
	// name of the program
	env_vector.push_back("SERVER_SOFTWARE=" + std::string(PROGNAME));
	// protocol
	env_vector.push_back("SERVER_PROTOCOL=" + _request.getHTTPVersion());

	// std::string encoding = request.peekHeader("transfer-encoding");

	env_vector.push_back("CONTENT_LENGTH=" + _request.peekHeader("content-length"));

	env_vector.push_back("CONTENT_TYPE=" + _request.peekHeader("content-type"));

	env_vector.push_back("AUTH_TYPE=");
	env_vector.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env_vector.push_back("REMOTE_ADDR=localhost");
	env_vector.push_back("REMOTE_HOST=");
	env_vector.push_back("REMOTE_IDENT=");
	env_vector.push_back("REDIRECT_STATUS=1");

	std::vector<char *> env_char;
	for (size_t i = 0; i < env_vector.size(); i++)
		env_char.push_back(const_cast<char *>(env_vector[i].c_str()));
	env_char.push_back(NULL);

	int sv[2]; // sv[0] is for parent, sv[1] is for child
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0)
	{
		std::cerr << "CGI Error: socketpair creation failed." << std::endl;
		return;
	}

	int cgi_pid = 0;


	cgi_pid = fork();

	if (cgi_pid < 0)
	{
		std::cerr << "CGI Error: fork failed." << std::endl;
		close(sv[0]);
		close(sv[1]);
		return;
	}

	if (cgi_pid == 0)
	{
		dup2(sv[1], STDIN_FILENO);
		dup2(sv[1], STDOUT_FILENO);

		close(sv[0]);
		close(sv[1]);


		std::vector<char *> argv;
		argv.reserve(_args.size() + 2);
		for (size_t i = 0; i < _args.size(); i++)
			argv.push_back(const_cast<char *>(_args[i].c_str()));
		argv.push_back(const_cast<char *>(script_full_path.c_str()));
		argv.push_back(NULL);

		execve(argv[0], &argv[0], env_char.data());
		std::cerr << "CGI Error: execve failed for " << argv[0] << ", " << argv[1] << ", " << argv[2] << std::endl;
		std::exit(1);
	}
	close(sv[1]);

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
	_cgi_io = new async::BufIo<>(_reactor, sv[0]);
	_status = _processing;
}

async::Future::Poll CGIBackend::process_io() throw(async::OsError)
{
	if (!_request_body_read_done && _request_body_read_index == _request_body_read_len)
	{
		container::Slice<uint8_t> slice(_request_body_buf, 4096);
		if (_io.poll_read(slice, _request_body_read_len) == Ready)
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
		if (_cgi_io->poll_write(slice, written) == Ready)
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
		if (_cgi_io->poll_read_line(line_buf) == Pending)
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
					if (key == "status" && !_status_set)
					{
						std::string status_value, status_code;
						parse_cgi_status_value(value, status_value, status_code);
						_response.setHTTPStatus(status_value, status_code, _status_pages);
						_status_set = true;
					}
					else
						_response.headAppend(std::string(line_buf.begin(), line_buf.end()));
				}
			}
		}
	}
	if (_request_body_read_done && _body_write_done && _response_headers_read_done)
	{
		if (!_status_set)
			_response.setHTTPStatus("200", "Ok", _status_pages);
		return Ready;
	}

	return Pending;
}

bool CGIBackend::parse_cgi_header(const std::vector<uint8_t> &raw_line, std::string &key, std::string &value)
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
		--key_end;

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

void CGIBackend::parse_cgi_status_value(const std::string &status_value, std::string &status_code, std::string &status_message)
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
