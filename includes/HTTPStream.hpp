#ifndef HTTPSTREAM_HPP
#define HTTPSTREAM_HPP

#include "Request.hpp"
#include <async>
#include <sys/socket.h>

enum stream_status
{
    _fetching_request,
    _getting_backend,
    _processing_response,
    _sending_response
};

class HTTPStream : public async::Future
{
  public:
    HTTPStream(async::Reactor &reactor, int fd, struct sockaddr_storage addr, socklen_t addr_len);
    HTTPStream(HTTPStream const &HTTPStream);
    ~HTTPStream();

    HTTPStream  &operator=(HTTPStream const &other);
    Future::Poll poll() throw(async::OsError);

    struct sockaddr_storage addr;
    socklen_t               addr_len;

  private:
    HTTPStream();
    Poll readRequest();
    Poll readFirstLine();
    Poll readHeaders();

    async::BufIo<>                      buf_io;
    stream_status                       _status;
    async::Reactor                     *_reactor;
    Request                             _request;
    bool                                _request_first_line_read;
    std::pair<std::string, std::string> _current_header;
};

#endif
