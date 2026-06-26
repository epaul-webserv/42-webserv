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

int main(int ac, char **av)
{
    int              sfd, s;
    struct addrinfo  hints;
    struct addrinfo *result, *rp;

    if (ac != 3)
    {
        std::cerr << "Usage: " << av[0] << "ip port\n";
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family    = AF_UNSPEC;
    hints.ai_socktype  = SOCK_STREAM;
    hints.ai_flags     = 0;
    hints.ai_protocol  = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr      = NULL;
    hints.ai_next      = NULL;

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
    s = getnameinfo(rp->ai_addr, rp->ai_addrlen, host, sizeof(host), port, sizeof(port),
                    NI_NUMERICHOST | NI_NUMERICSERV);
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
    async::Reactor           reactor;
    Router                   router;
    RouterNode               test;
    std::vector<std::string> php_args;
    php_args.push_back("/bin/php-cgi");
    php_args.push_back("-f");
    std::pair<t_backend_type, std::vector<std::string> > backend_pair(_cgi, php_args);
    std::pair<std::string, std::pair<t_backend_type, std::vector<std::string> > > backend_rule(
        "php", backend_pair);
    test.backend_rules.push_back(backend_rule);
    // test.backend_rules.insert(std::pair<std::string, std::pair<t_backend_type,
    // std::vector<std::string> > >( 	"txt", std::pair<t_backend_type, std::vector<std::string>
    // >(_upload, std::vector<std::string>())));
    test.accepted_methods.push_back("GET");
    test.accepted_methods.push_back("POST");
    test.file_upload = _true;
    test.redirects.insert(std::pair<std::string, std::pair<std::string, e_redirect_type> >(
        "/logan",
        std::pair<std::string, e_redirect_type>("https://www.loganmartin.fr/", _temporary)));
    test.status_pages["201"] = "/index.nginx-debian.html";
    test.list_directories    = _true;
    test.index_pages.push_back("index.nginx-debian.html");

    router.addServ("localhost", test);
    MimeType::setType("png", "image/png");
    std::cout << MimeType::getType("testpng") << std::endl;
    Acceptor acceptor(reactor, sfd, router);
    reactor.run(acceptor);
    // close(socketpoll); FIXME: We would like to close the epoll_create1 fd
}
