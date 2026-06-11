#include "CGIBackend.hpp"
#include <iostream>
#include <string>
#include <vector>
#include "url_utils.hpp"
#include "webserv.h"


// int WriteBody(const std::string &body, int write_fd)
// {
// 	size_t	written	  = 0;
// 	ssize_t just_read = 0;

// 	while (written != body.length())
// 	{
// 		just_read = write(1, &body.c_str()[written], body.length() - written);
// 		if (just_read == -1)
// 			return -1;
// 		written += just_read;
// 	}
// 	return 1;
// }

int ReadBody(Response &response, int read_fd)
{
	(void)response;
	ssize_t just_read = 1;
	char	buff[256];
	while (just_read > 0)
	{
		just_read = read(read_fd, buff, 256);
		if (just_read < 0)
			return -1;
		if (just_read > 0)
			write(1, buff, just_read);
	}
	return 1;
}


CGIBackend::CGIBackend(void) {}

CGIBackend::CGIBackend(std::vector<std::string> const &args) : _args(args) {}

CGIBackend::~CGIBackend(void) {}

void CGIBackend::generateResponse(Request const &request, Response &response, async::Reactor &reactor, async::BufIo<> &bodyIo)
{
	if (response.getStatus() == _init)
	{
		initResponse(request, response, reactor, bodyIo);
	}
	if (response.getStatus() == _pending)
		response.pollGenerator();
}

// The request has been routed towards the cgi, we start the program and setup
// the io to process it
void CGIBackend::initResponse(Request const &request, Response &response, async::Reactor &reactor, async::BufIo<> &bodyIo)
{
	std::string				 encoding = request.peekHeader("transfer-encoding");
	std::vector<std::string> env_vector;

	env_vector.push_back("QUERY_STRING=" + request.getQueryString());
	env_vector.push_back("REQUEST_METHOD=" + request.getMethod());

	// The name of the file being executed
	std::string script_name		 = get_file_name(request.getURI());
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
	env_vector.push_back("SERVER_PROTOCOL=" + request.getHTTPVersion());

	// std::string encoding = request.peekHeader("transfer-encoding");

	env_vector.push_back("CONTENT_LENGTH=" + request.peekHeader("content-length"));

	env_vector.push_back("CONTENT_TYPE=" + request.peekHeader("content-type"));

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

	response.setGenerator(request, response, reactor, bodyIo, sv[0]);
	response.setStatus(_pending);
}
