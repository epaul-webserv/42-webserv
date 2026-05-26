#ifndef WEBSERV_H
# define WEBSERV_H

# define PROGNAME "Webserv"
# define MAX_REQUEST_SIZE 8192
# define MAX_CONN 1000

# include <fstream>
# include <sstream>
# include <sys/socket.h>
# include <iostream>
# include <cstring>
# include <cstdlib>
# include <arpa/inet.h>
# include <unistd.h>
# include <string>
# include <poll.h>
# include <sys/epoll.h>
# include <signal.h>
# include "default_conf.hpp"
# include "RequestPool.hpp"
# include "html_generator.hpp"
# include "Date.hpp"

std::string get_html_dir(std::string const &path, std::string const &root);
std::string get_html_error(std::string const & status,
							std::string const &message);
void get_static_file(std::string const &path, std::string const &root, int fd);
int  init_listener(struct sockaddr_in &addr, socklen_t &addrlen);
void prepare_response(Request request);

// URL_ENCODING
void decodeURL(std::string &URI);

#endif
