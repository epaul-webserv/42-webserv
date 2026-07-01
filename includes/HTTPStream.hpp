#ifndef HTTPSTREAM_HPP
#define HTTPSTREAM_HPP

#include "IRequestBody.hpp"
#include "IResponseBody.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Router.hpp"
#include "string_overload.hpp"
#include <async>
#include <iostream>
#include <limits>
#include <sys/socket.h>

class Router;

enum stream_status
{
    _fetching_request,
    _fetching_headers,
    _getting_backend,
    _processing_response
};

enum headers_status
{
    _initializing_headers,
    _sending_headers,
    _headers_sent
};

class HTTPStream : public async::Future
{
  public:
    HTTPStream(async::Reactor &reactor, int fd, const struct sockaddr_storage &addr,
               socklen_t addr_len, Router &router);
    async::Future::Poll readRequest();
    ~HTTPStream();

    HTTPStream         &operator=(HTTPStream const &other);
    Future::Poll        poll() throw(async::OsError);
    async::Future::Poll readFirstLine();
    async::Future::Poll readHeaders();
    async::Future::Poll consume_body();
    Future::Poll        keepAlive();
    void                setHTTPStatus(std::string const &status, std::string const &message);
    void                setErrorResponse(std::string const &status, std::string const &message);
    void                setBrokenConnection();
    void                keepConnection();
    bool                setStatusResponse();
    IRequestBody       *request_body();
    async::Future::Poll poll_send_headers(IResponseBody *&b);
    async::Future::Poll poll_send_headers(ssize_t content_length, IResponseBody *&b);

    Response                           response;
    Request                            request;
    struct sockaddr_storage            addr;
    socklen_t                          addr_len;
    std::map<std::string, std::string> status_pages;
    Router                            *_router;
    ssize_t                            _max_resource_size;

  private:
    HTTPStream();
    HTTPStream(HTTPStream const &other);
    Poll sendHeaders() throw(async::OsError);

    container::RingBuf<uint8_t, 65535> buf;

  private:
    bool                                _override_response;
    bool                                _rerun;
    bool                                _broken_connection;
    bool                                _keep_connection;
    bool                                _body_read;
    std::string                         _saved_status;
    std::string                         _saved_msg;
    stream_status                       _status;
    async::Reactor                     *_reactor;
    ABackend                           *_backend;
    headers_status                      _headers_status;
    std::vector<uint8_t>                _header_line;
    size_t                              _headers_sent_bytes;
    async::BufIo<65535>                 buf_io;
    std::pair<std::string, std::string> _current_header;
};

#endif
