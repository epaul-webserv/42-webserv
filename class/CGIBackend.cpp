#include "CGIBackend.hpp"
#include <iostream>
#include <string>


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

CGIBackend::CGIBackend(std::string const &path) : _path(path) {}

CGIBackend::~CGIBackend(void) {}

// The request has been routed towards the cgi, we start the program and setup
// the io to process it
void CGIBackend::generateResponse(Request const &request, Response &response)
{
	(void)request;
	(void)response;
	char *const env_var[] = {
		const_cast<char *>("AUTH_TYPE="), // NULL ?
		const_cast<char *>("CONTENT_TYPE="), // If there is a body
		const_cast<char *>("CONTENT_LENGTH="), // If there is a body
		const_cast<char *>("GATEWAY_INTERFACE=CGI/1.1"), // default to 1.1

		//	SCHEME			AUTHORITY			SCRIPT_NAME PATH_INFO
		// QUERY_STRING
		// 	http	://		example.com:80412 	/cgi-bin/search.cgi
		// /archive/docs	?year=2026&name=toto
		const_cast<char *>("PATH_INFO=/archive/docs"),

		// http://somehost.com/cgi-bin/somescript/this%2eis%2epath%3binfo
		/// usr/local/www/htdocs/this.is.the.path;info < this is translated, an
		/// internal decoded URI
		const_cast<char *>(
			"PATH_TRANSLATED=/home/bl4st/github/webserv/index.php"), //

		const_cast<char *>(
			"QUERY_STRING=year=2026&name=toto"), // the content after ? in the
												 // url

		const_cast<char *>("REMOTE_ADDR=localhost"), // ip address of the client
													 // making the request
		const_cast<char *>(
			"REMOTE_HOST="), // fully qualified domain name of, the client
							 // sending the request left blank i guess
		const_cast<char *>("REMOTE_IDENT="), // Idk

		const_cast<char *>("REQUEST_METHOD=GET"), // Get from request
		const_cast<char *>(
			"REDIRECT_STATUS=1"), // Bypasses the "Security Alert!"
		const_cast<char *>("SCRIPT_FILENAME=/home/bl4st/github/webserv/"
						   "index.php"), // Tells PHP exactly what
										 // file to run
		const_cast<char *>(
			"SCRIPT_NAME=index.php"), // Path to the script to execute
		const_cast<char *>("SERVER_NAME=webserv_name"), //
		const_cast<char *>("SERVER_PORT=8080"), //
		const_cast<char *>("SERVER_PROTOCOL=HTTP/1.1"), //
		const_cast<char *>("SERVER_SOFTWARE=webserv"), //
		NULL}; //

	int pipe_in[2];
	int pipe_out[2];

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

		char *const argv[] = {const_cast<char *>("/bin/php-cgi"),
							  const_cast<char *>("-f"),
							  const_cast<char *>("index.php"), NULL};

		execve(argv[0], argv, env_var);
		std::cerr << "CGI Error: execve failed for " << argv[0] << std::endl;
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
