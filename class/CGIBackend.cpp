#include "CGIBackend.hpp"
#include <iostream>
#include <string>
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

// The request has been routed towards the cgi, we start the program and setup
// the io to process it
void CGIBackend::generateResponse(Request const &request, Response &response, async::Reactor &reactor, async::BufIo<> &io)
{
	(void)request;
	(void)response;
	(void)reactor;
	(void)io;
	std::string query_string   = "QUERY_STRING=" + request.getQueryString();
	std::string request_method = "REQUEST_METHOD=" + request.getMethod();

	// The name of the file being executed
	std::string script_name		 = get_file_name(request.getURI());
	std::string script_full_path = std::string(DEFAULT_ROOT) + "/" + script_name;

	// This is supposed to be whats after the script filename, and before the query string,
	// but Logan Martin told me to trust him, "tkt" was his explanation, it convinced me
	std::string path_info = "PATH_INFO=" + script_full_path;
	// Internal decoded URI
	std::string path_translated = "PATH_TRANSLATED=" + script_full_path;
	// Tells the cgi EXACTLY what file to run
	std::string script_filename = "SCRIPT_FILENAME=" + script_full_path;

	std::string server_port = "SERVER_PORT=" + DEFAULT_PORT;

	// name of teh server on the network ?
	std::string server_name = "SERVER_NAME=" + std::string(PROGNAME);
	// name of the program
	std::string server_software = "SERVER_SOFTWARE=" + std::string(PROGNAME);
	// protocol
	std::string server_protocol = "SERVER_PROTOCOL=" + request.getHTTPVersion();

	char *const env_var[] = {const_cast<char *>("AUTH_TYPE="), // NULL ?
							 const_cast<char *>("CONTENT_TYPE="), // If there is a body
							 const_cast<char *>("CONTENT_LENGTH="), // If there is a body
							 const_cast<char *>("GATEWAY_INTERFACE=CGI/1.1"), // default to 1.1

							 const_cast<char *>(path_info.c_str()),

							 const_cast<char *>(path_translated.c_str()),

							 const_cast<char *>(query_string.c_str()), // the content after ? in the
																	   // url

							 const_cast<char *>("REMOTE_ADDR=localhost"), // ip address of the client
																		  // making the request
							 const_cast<char *>("REMOTE_HOST="), // fully qualified domain name of, the client
																 // sending the request left blank i guess
							 const_cast<char *>("REMOTE_IDENT="), // Idk

							 const_cast<char *>(request_method.c_str()), // Get from request

							 const_cast<char *>("REDIRECT_STATUS=1"), // Bypasses the "Security Alert!"
							 const_cast<char *>(script_filename.c_str()),
							 const_cast<char *>(script_name.c_str()), // Path to the script to execute
							 const_cast<char *>(server_name.c_str()), //
							 const_cast<char *>(server_port.c_str()), //
							 const_cast<char *>(server_protocol.c_str()), //
							 const_cast<char *>(server_software.c_str()), //
							 NULL}; //
	int			pipe_in[2];
	int			pipe_out[2];

	int cgi_pid = 0;

	if (pipe(pipe_in) < 0)
	{
		std::cerr << "CGI Error: pipe creation failed." << std::endl;
		return;
	}
	if (pipe(pipe_out) < 0)
	{
		close(pipe_in[0]);
		close(pipe_in[1]);
		std::cerr << "CGI Error: pipe creation failed." << std::endl;
		return;
	}


	cgi_pid = fork();
	if (cgi_pid < 0)
	{
		std::cerr << "CGI Error: fork failed." << std::endl;
		close(pipe_in[0]);
		close(pipe_in[1]);
		close(pipe_out[0]);
		close(pipe_out[1]);
		return;
	}

	if (cgi_pid == 0)
	{
		dup2(pipe_in[0], STDIN_FILENO);
		dup2(pipe_out[1], STDOUT_FILENO);

		close(pipe_in[0]);
		close(pipe_in[1]);
		close(pipe_out[0]);
		close(pipe_out[1]);


		std::vector<char *> argv;
		argv.reserve(_args.size() + 2);
		for (size_t i = 0; i < _args.size(); i++)
			argv.push_back(const_cast<char *>(_args[i].c_str()));
		argv.push_back(const_cast<char *>(script_full_path.c_str()));
		argv.push_back(NULL);

		execve(argv[0], &argv[0], env_var);
		std::cerr << "CGI Error: execve failed for " << argv[0] << ", " << argv[1] << ", " << argv[2] << std::endl;
		std::exit(1);
	}
	close(pipe_in[0]);
	close(pipe_out[1]);

	int cgi_in_fd  = pipe_in[1];
	int cgi_out_fd = pipe_out[0];

	/*if (WriteBody("body", cgi_in_fd) < 0)
	{
		close(cgi_in_fd);
		close(cgi_out_fd);
		return;
	}*/
	close(cgi_in_fd);
	if (ReadBody(response, cgi_out_fd) < 0)
	{
		close(cgi_out_fd);
		return;
	}
	close(cgi_out_fd);
	return;
}
