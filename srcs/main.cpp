#include "Request.hpp"
#include "Router.hpp"
#include "StaticBackend.hpp"
#include "webserv.h"

int g_sig = 0;

int get_conn_id(int connexion)
{
	static int	ids[MAX_CONN] = {};
	static bool init		  = false;

	if (!init)
	{
		memset(ids, -1, sizeof(ids));
		init = true;
	}
	if (connexion < 0)
	{
		for (int i = 0; i < MAX_CONN; ++i)
		{
			if (connexion == -ids[i])
			{
				ids[i] = -1;
				return (-1);
			}
		}
	}
	for (int i = 0; i < MAX_CONN; ++i)
	{
		if (connexion == ids[i])
			return (i);
	}
	for (int i = 0; i < MAX_CONN; ++i)
	{
		if (ids[i] == -1)
		{
			ids[i] = connexion;
			return (i);
		}
	}
	return (-1);
}

void new_conn(int sockfd, struct sockaddr_in &addr, socklen_t &addrlen,
			  int &socketpoll, RequestPool &pool)
{
	// std::cout << "[DEBUG] Creating new connexion " << std::endl;
	struct epoll_event event;
	int connexion = accept(sockfd, (struct sockaddr *)&addr, &addrlen);
	if (connexion < 0)
	{
		close(sockfd);
		std::cerr << "Error on accept : " << std::strerror(errno) << std::endl;
		exit(EXIT_FAILURE);
	}
	event.events  = EPOLLIN | EPOLLHUP;
	event.data.fd = connexion;
	epoll_ctl(socketpoll, EPOLL_CTL_ADD, connexion, &event);
	pool.getRequest(connexion);
	// std::cout << "[DEBUG] Connexion successfully created" << std::endl;
}

static void wait_to_send(int const &socketpoll, int const &connfd)
{
	struct epoll_event event;

	event.events  = EPOLLOUT | EPOLLHUP;
	event.data.fd = connfd;
	epoll_ctl(socketpoll, EPOLL_CTL_MOD, connfd, &event);
}

// static void wait_to_read(int const &socketpoll, int const &connfd)
// {
//     struct epoll_event  event;
//
//     event.events = EPOLLIN | EPOLLHUP;
//     event.data.fd = connfd;
//     epoll_ctl(socketpoll, EPOLL_CTL_MOD, connfd, &event);
// }

void handle_events(struct epoll_event *events, struct sockaddr_in &addr,
				   socklen_t &addrlen, int &socketpoll, int sockfd,
				   RequestPool &pool, Router &router)
{
	StaticBackend static_backend;
	Response	  response(-1);

	while (true)
	{
		if (int nbev = epoll_wait(socketpoll, events, MAX_CONN, -1))
		{
			// std::cout << "nv events : " << nbev << std::endl;
			if (g_sig == SIGINT)
				break;
			for (int i = 0; i < nbev; ++i)
			{
				socklen_t test_len;
				getsockname(events[i].data.fd, (struct sockaddr *)&addr,
							&test_len);
				std::cerr << inet_ntoa(addr.sin_addr) << std::endl;
				;
				if (events[i].data.fd == sockfd && events[i].events & EPOLLIN)
					new_conn(events[i].data.fd, addr, addrlen, socketpoll,
							 pool);
				else if (events[i].data.fd != sockfd &&
						 events[i].events & EPOLLIN)
				{
					t_request_state s =
						pool.getRequest(events[i].data.fd).readRequest();
					if (s == _read)
					{
						response.set(events[i].data.fd);
						router.resolve(pool.getRequest(events[i].data.fd),
									   response);
						// static_backend.generateResponse(
						// 	pool.getRequest(events[i].data.fd), response);
						// pool.getRequest(events[i].data.fd).prepareResponse();
						if (response.ready())
							wait_to_send(socketpoll, events[i].data.fd);
					}
					if (s == _close)
					{
						epoll_ctl(socketpoll, EPOLL_CTL_DEL, events[i].data.fd,
								  NULL);
						close(events[i].data.fd);
						pool.delRequest(events[i].data.fd);
					}
				}
				else if (events[i].events & EPOLLOUT)
				{
					if (response.sendResponse())
					{
						// if
						// (pool.getRequest(events[i].data.fd).sendResponse())
						// {
						epoll_ctl(socketpoll, EPOLL_CTL_DEL, events[i].data.fd,
								  NULL);
						close(events[i].data.fd);
						pool.delRequest(events[i].data.fd);
					}
				}
				else if (events[i].events & EPOLLHUP)
				{
					std::cout << "COMMENT CA ?" << std::endl;
					close(events[i].data.fd);
					pool.delRequest(events[i].data.fd);
				}
			}
		}
	}
}

void sigint_handler(int sig)
{
	(void)sig;
	g_sig = sig;
}

int main(int ac, char *av[])
{
	RequestPool pool;
	Router		router;

	router.addRule(_cgi, std::vector<std::string>(1, "/bin/python3"), "localhost:33000", "py");

	std::vector<std::string> php_args;
	php_args.push_back("/bin/php-cgi");
	php_args.push_back("-f");
	router.addRule(_cgi, php_args, "localhost:33000", "php");

	if (ac > 2)
	{
		std::cerr << "Usage: " << av[0] << " [configuration file]" << std::endl;
		return (1);
	}
	if (ac == 1)
	{
	}
	else if (ac == 2)
	{
	}
	signal(SIGINT, sigint_handler);
	struct sockaddr_in addr = {};
	addr.sin_family			= AF_INET;
	addr.sin_addr.s_addr	= INADDR_ANY;
	addr.sin_port			= htons(DEFAULT_PORT);
	socklen_t addrlen		= sizeof(addr);
	int		  sockfd		= init_listener(addr, addrlen);
	if (sockfd < 0)
	{
		std::cerr << std::strerror(errno) << std::endl;
		return (EXIT_FAILURE);
	}
	int				   socketpoll = epoll_create1(EPOLL_CLOEXEC);
	struct epoll_event sockevent, events[MAX_CONN];
	sockevent.events  = EPOLLIN | EPOLLHUP;
	sockevent.data.fd = sockfd;
	epoll_ctl(socketpoll, EPOLL_CTL_ADD, sockfd, &sockevent);
	// chdir("/var/www/html");
	handle_events(events, addr, addrlen, socketpoll, sockfd, pool, router);
	if (sockfd > 0)
	{
		shutdown(sockfd, SHUT_RDWR);
		close(sockfd);
	};
	if (socketpoll > 0)
	{
		close(socketpoll);
	};
	std::cout << "\nBONSOIR PARIIIIIS!" << std::endl;
}
