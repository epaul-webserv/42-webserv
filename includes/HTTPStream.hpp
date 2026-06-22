#ifndef HTTPSTREAM_HPP
#define HTTPSTREAM_HPP

#include "Request.hpp"
#include "Response.hpp"
#include "Router.hpp"
#include <async>
#include <sys/socket.h>

class Router;

enum stream_status
{
    _fetching_request,
    _fetching_headers,
    _getting_backend,
    _processing_response,
    _sending_response
};

class HTTPStream : public async::Future
{
  public:
    HTTPStream(async::Reactor &reactor, int fd, struct sockaddr_storage addr, socklen_t addr_len,
               Router *router);
    HTTPStream(HTTPStream const &other);
    ~HTTPStream();

    HTTPStream  &operator=(HTTPStream const &other);
    Future::Poll poll() throw(async::OsError);

    struct sockaddr_storage addr;
    socklen_t               addr_len;
    Request                 request;
    Response                response;

  private:
    HTTPStream();
    Poll readRequest();
    Poll readFirstLine();
    Poll readHeaders();

    async::BufIo<>                      buf_io;
    stream_status                       _status;
    async::Reactor                     *_reactor;
    Router                             *_router;
    std::pair<std::string, std::string> _current_header;
    ABackend                           *_backend;
};

#endif
