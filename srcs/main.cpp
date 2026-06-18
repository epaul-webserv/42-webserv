#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include <async>
#include "HTTPStream.hpp"
#include "Router.hpp"
#include "webserv.h"

class Acceptor : public async::Future
{
	async::Io				accept;
	async::Reactor		   &reactor;
	std::vector<HTTPStream> clients;
	Router				   &router;

public:
	Acceptor(async::Reactor &reactor, int fd, Router &router) : accept(reactor, fd), reactor(reactor), router(router) {}

	Future::Poll poll() throw(async::OsError)
	{
		int						fd;
		struct sockaddr_storage addr	 = {};
		socklen_t				addr_len = sizeof(addr);
		char					host[1024];
		char					port[32];

		while (accept.poll_accept(reinterpret_cast<struct sockaddr *>(&addr), &addr_len, fd) == Future::Ready)
		{
			int s = getnameinfo(reinterpret_cast<struct sockaddr *>(&addr), addr_len, host, sizeof(host), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
			if (s != 0)
			{
				std::cerr << "getnameinfo: " << gai_strerror(s) << "\n";
				exit(EXIT_FAILURE);
			}
			std::cout << host << ":" << port << " is connected\n";
			clients.push_back(HTTPStream(reactor, fd, addr, addr_len, &router));
		}
		for (long i = clients.size() - 1; i >= 0; i--)
		{
			Poll poll = async::Future::Pending;
			try
			{
				poll = clients[i].poll();
			}
			catch (async::OsError &e)
			{
				int s = getnameinfo((struct sockaddr *)&clients[i].addr, clients[i].addr_len, host, sizeof(host), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
				if (s != 0)
				{
					std::cerr << "getnameinfo: " << gai_strerror(s) << "\n";
					exit(EXIT_FAILURE);
				}
				std::cout << host << ":" << port << " left\n";
				std::cerr << "Error " << host << ":" << port << ": " << e.what() << " (os error " << e.errrno() << ")\n";
			}
			if (poll == Future::Ready)
			{
				int s = getnameinfo((struct sockaddr *)&clients[i].addr, clients[i].addr_len, host, sizeof(host), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
				if (s != 0)
				{
					std::cerr << "getnameinfo: " << gai_strerror(s) << "\n";
					exit(EXIT_FAILURE);
				}
				std::cout << host << ":" << port << " left\n";
				clients.erase(clients.begin() + i);
			}
		}
		return Future::Pending;
	}
};

#include <toml>

int main(int ac, char **av)
{
	int	   sfd = -1, s;
	Router router;
	// socklen_t peer_addrlen;
	struct addrinfo								  hints = {};
	struct addrinfo								 *result, *rp;
	std::map<std::pair<std::string, int>, Router> router_vec;
	// struct sockaddr_storage peer_addr;

	if (ac != 3)
	{
		std::cerr << "Usage: " << av[0] << "ip port\n";
		exit(EXIT_FAILURE);
	}

	std::vector<std::string> php_args;
	php_args.push_back("/bin/php-cgi");
	php_args.push_back("-f");

	RouterNode																	  test;
	std::pair<t_backend_type, std::vector<std::string> >						  backend_pair(_cgi, php_args);
	std::pair<std::string, std::pair<t_backend_type, std::vector<std::string> > > backend_rule("php", backend_pair);
	test.backend_rules.insert(backend_rule);
	// test.backend_rules.insert(std::pair<std::string, std::pair<t_backend_type, std::vector<std::string> > >(
	// 	"txt", std::pair<t_backend_type, std::vector<std::string> >(_upload, std::vector<std::string>())));
	test.accepted_methods.push_back("GET");
	test.accepted_methods.push_back("POST");
	test.file_upload = _true;
	test.redirects.insert(std::pair<std::string, std::pair<std::string, e_redirect_type> >(
		"/logan", std::pair<std::string, e_redirect_type>("https://www.loganmartin.fr/", _temporary)));
	test.status_pages["201"] = "/index.nginx-debian.html";
	test.list_directories	 = _true;


	router.addServ("localhost", test);

	MimeType::mime_type["html"] = "text/html";
	MimeType::mime_type["png"]	= "image/png";
	MimeType::mime_type["jpg"]	= "image/jpg";
	MimeType::mime_type["svg"]	= "image/svg+xml";
	MimeType::mime_type["mkv"]	= "video/x-matroska";
	MimeType::mime_type["txt"]	= "text/plain";
	MimeType::mime_type["ico"]	= "image/x-icon";

	memset(&hints, 0, sizeof(hints));
	hints.ai_family	   = AF_UNSPEC;
	hints.ai_socktype  = SOCK_STREAM;
	hints.ai_flags	   = 0;
	hints.ai_protocol  = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr	   = NULL;
	hints.ai_next	   = NULL;

	s = getaddrinfo(av[1], av[2], &hints, &result);
	if (s != 0)
	{
		std::cerr << "getaddrinfo: " << gai_strerror(s) << "\n";
		exit(EXIT_FAILURE);
	}

	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
			continue;
		int opt = 1;
		if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		{
			std::cerr << "setsockopt: " << strerror(errno) << "\n";
			exit(EXIT_FAILURE);
		}
		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
			break;
		close(sfd);
	}

	if (rp == NULL)
	{
		std::cerr << "Could not bind\n";
		freeaddrinfo(result);
		exit(EXIT_FAILURE);
	}
	char host[1024];
	char port[32];
	s = getnameinfo(rp->ai_addr, rp->ai_addrlen, host, sizeof(host), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
	freeaddrinfo(result);

	if (s != 0)
	{
		std::cerr << "getnameinfo: " << gai_strerror(s) << "\n";
		exit(EXIT_FAILURE);
	}

	if (listen(sfd, 100) < 0)
	{
		std::cerr << "listen: " << strerror(errno) << "\n";
		exit(EXIT_FAILURE);
	}

	std::cout << "Listening on " << host << ":" << port << "\n";
	async::Reactor reactor;
	Acceptor	   acceptor(reactor, sfd, router);
	reactor.run(acceptor);
	shutdown(sfd, SHUT_RDWR);
	close(sfd);
	// close(socketpoll); We would like to close the epoll_create1 fd
}
