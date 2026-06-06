#ifndef WEBSERV_H
#define WEBSERV_H

#define PROGNAME "Webserv"
#define MAX_REQUEST_SIZE 8192
#define MAX_CONN 1000

#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <poll.h>
#include <signal.h>
#include <sstream>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include "Date.hpp"
#include "default_conf.hpp"
#include "html_generator.hpp"

std::string get_html_dir(std::string const &path, std::string const &root);
std::string get_html_error(std::string const &status, std::string const &message);
void		get_static_file(std::string const &path, std::string const &root, int fd);
int			init_listener(struct sockaddr_in &addr, socklen_t &addrlen);

#endif
