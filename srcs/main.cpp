#include <cerrno>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "ConfigParser.hpp"
#include "HTTPStream.hpp"
#include "MimeType.hpp"
#include "Router.hpp"
#include <async>

class Acceptor : public async::Future
{
    async::Io               accept;
    async::Reactor         &reactor;
    std::vector<HTTPStream> clients;
    int                     sfd;
    Router                  router;

  public:
    Acceptor(async::Reactor &reactor, int fd, Router router)
        : accept(reactor, fd), reactor(reactor), sfd(fd), router(router)
    {
    }
    ~Acceptor() { shutdown(sfd, SHUT_RDWR); }

    Future::Poll poll() throw(async::OsError)
    {
        int                     fd;
        struct sockaddr_storage addr;
        socklen_t               addr_len = sizeof(addr);
        char                    host[1024];
        char                    port[32];

        while (accept.poll_accept(reinterpret_cast<struct sockaddr *>(&addr), &addr_len, fd) ==
               Future::Ready)
        {
            int s = getnameinfo(reinterpret_cast<struct sockaddr *>(&addr), addr_len, host,
                                sizeof(host), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
            if (s != 0)
            {
                std::cerr << "getnameinfo: " << gai_strerror(s) << "\n";
                exit(EXIT_FAILURE);
            }
            std::cout << host << ":" << port << " is connected\n";
            clients.push_back(HTTPStream(reactor, fd, addr, addr_len, router));
        }
        for (size_t i = 0; i < clients.size(); i++)
        {
            Poll poll;
            try
            {
                poll = clients[i].poll();
            }
            catch (async::OsError &e)
            {
                int s =
                    getnameinfo((struct sockaddr *)&clients[i].addr, clients[i].addr_len, host,
                                sizeof(host), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
                if (s != 0)
                {
                    std::cerr << "getnameinfo: " << gai_strerror(s) << "\n";
                    exit(EXIT_FAILURE);
                }
                std::cout << host << ":" << port << " left\n";
                std::cerr << "Error " << host << ":" << port << ": " << e.what() << " (os error "
                          << e.errrno() << ")\n";
            }
            if (poll == Future::Ready)
            {
                int s = getnameinfo(reinterpret_cast<struct sockaddr *>(&clients[i].addr),
                                    clients[i].addr_len, host, sizeof(host), port, sizeof(port),
                                    NI_NUMERICHOST | NI_NUMERICSERV);
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

class MultiAcceptor : public async::Future
{
    std::vector<Acceptor *> acceptors;

  private:
    MultiAcceptor &operator=(MultiAcceptor &other);
    MultiAcceptor();
    MultiAcceptor(const MultiAcceptor &other);

  public:
    MultiAcceptor(std::vector<Acceptor *> &acceptors) : acceptors(acceptors) {}
    ~MultiAcceptor()
    {
        for (size_t i = 0; i < acceptors.size(); ++i)
        {
            delete acceptors[i];
        }
    }

    Future::Poll poll() throw(async::OsError)
    {
        for (size_t i = 0; i < acceptors.size(); ++i)
        {
            acceptors[i]->poll();
        }
        return Future::Pending;
    }
};

int main(int ac, char **av)
{

    // TODO: router_map keys must bee freed with freeaddrinfo when exiting teh program
    std::map<struct addrinfo *, Router> router_map;

    if (ac != 2)
    {
        std::cerr << "Usage: " << av[0] << " config.toml\n";
        return (1);
    }

    try
    {
        ConfigParser::ParseConfig(router_map, av[1]);
        // printRouterMap(router_map);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in config: \n\t" << e.what() << '\n';
        return (1);
    }

    int sfd = -1, s;

    if (router_map.size() == 0)
        return (1);

    async::Reactor                                reactor;
    std::vector<Acceptor *>                       acceptors;
    std::vector<int>                              sockets;
    std::map<struct addrinfo *, Router>::iterator it;

    for (it = router_map.begin(); it != router_map.end(); ++it)
    {
        struct addrinfo *ai       = it->first;
        Router          &router   = it->second;
        int              bound_fd = -1;

        for (struct addrinfo *rp = ai; rp != NULL; rp = rp->ai_next)
        {
            sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (sfd == -1)
                continue;
            int opt = 1;
            if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
            {
                std::cerr << "setsockopt: " << strerror(errno) << "\n";
                close(sfd);
                continue;
            }
            if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
            {
                bound_fd = sfd;
                char host[1024];
                char port[32];
                s = getnameinfo(rp->ai_addr, rp->ai_addrlen, host, sizeof(host), port, sizeof(port),
                                NI_NUMERICHOST | NI_NUMERICSERV);
                if (s != 0)
                {
                    std::cerr << "getnameinfo: " << gai_strerror(s) << "\n";
                    close(sfd);
                    continue;
                }

                if (listen(sfd, 100) < 0)
                {
                    std::cerr << "listen: " << strerror(errno) << "\n";
                    close(sfd);
                    continue;
                }

                std::cout << "Listening on " << host << ":" << port << "\n";
                break;
            }
            close(sfd);
        }

        if (bound_fd == -1)
        {
            std::cerr << "Could not bind to any address\n";
            freeaddrinfo(ai);
            continue;
        }

        sockets.push_back(bound_fd);
        acceptors.push_back(new Acceptor(reactor, bound_fd, router));
        freeaddrinfo(ai);
    }

    if (acceptors.empty())
    {
        std::cerr << "No acceptors created\n";
        return (1);
    }

    MultiAcceptor multi_acceptor(acceptors);
    reactor.run(multi_acceptor);

    for (size_t i = 0; i < sockets.size(); ++i)
    {
        shutdown(sockets[i], SHUT_RDWR);
        close(sockets[i]);
    }
    // close(socketpoll); We would like to close the epoll_create1 fd
}
