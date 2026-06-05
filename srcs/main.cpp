#include <cerrno>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include <async>
#include "HTTPStream.hpp"
#include "Router.hpp"

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
		struct sockaddr_storage addr;
		socklen_t				addr_len = sizeof(addr);
		char					host[1024];
		char					port[32];


		while (accept.poll_accept((struct sockaddr *)&addr, &addr_len, fd) == Future::Ready)
		{
			int s = getnameinfo((struct sockaddr *)&addr, addr_len, host, sizeof(host), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
			if (s != 0)
			{
				std::cerr << "getnameinfo: " << gai_strerror(s) << "\n";
				exit(EXIT_FAILURE);
			}
			std::cout << host << ":" << port << " is connected\n";
			clients.push_back(HTTPStream(reactor, fd, addr, addr_len, &router));
		}
		for (size_t i = 0; i < clients.size(); i++)
		{
			if (clients[i].poll() == Future::Ready)
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

int main(int ac, char **av)
{
	int	   sfd, s;
	Router router;
	// socklen_t peer_addrlen;
	struct addrinfo	 hints;
	struct addrinfo *result, *rp;
	// struct sockaddr_storage peer_addr;

	if (ac != 3)
	{
		std::cerr << "Usage: " << av[0] << "ip port\n";
		exit(EXIT_FAILURE);
	}

	std::vector<std::string> php_args;
	php_args.push_back("/bin/php-cgi");
	php_args.push_back("-f");
	router.addRule(_cgi, php_args, "localhost:33000", "php");

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

	int opt = 1;
	if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		std::cerr << "setsockopt: " << strerror(errno) << "\n";
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
}
