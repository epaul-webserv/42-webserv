#ifndef CGI_BACKEND_HPP
#define CGI_BACKEND_HPP

#include "ABackend.hpp"
#include <async>
#include <vector>

#include "HTTPStream.hpp"

class CGIBackend : public ABackend
{
  public:
    CGIBackend(std::vector<std::string> const &args, std::string const &root,
               async::Reactor &reactor, HTTPStream &stream);
    ~CGIBackend(void);

    // This must be called once Ready has been sent
    Poll poll();

  private:
    enum status
    {
        _getting_request_body,
        _init,
        _processing
    };
    CGIBackend(const CGIBackend &other);
    CGIBackend &operator=(const CGIBackend &other);

    bool parse_cgi_header(const std::vector<uint8_t> &raw_line, std::string &key,
                          std::string &value);
    void parse_cgi_status_value(const std::string &status_value, std::string &status_code,
                                std::string &status_message);
    bool check_files_validity(const char *cgi_name, const char *script_name);
    async::Future::Poll get_body();
    void                init();
    async::Future::Poll copy_to_cgi();
    void                check_process_status();
    void                send_client_body_to_cgi();
    void                read_cgi_headers();
    async::Future::Poll set_body_writer();
    void                send_body();

    std::string const        _root;
    async::Reactor          &_reactor;
    std::vector<std::string> _args;
    async::BufIo<>          *_cgi_io;
    int                      _socket_fd;
    HTTPStream              &stream;
    std::string              headers;
    IResponseBody           *response_body;

    int _cgi_pid;

    uint8_t _body_read_buf[65535];
    size_t  _body_read_buf_len;
    size_t  _body_read_buf_index;
    bool    _client_body_sent;

    bool    _response_headers_read_done;
    bool    _status_set;
    bool    _process_done;
    ssize_t _content_length;

    status                                       _status;
    IRequestBody                                *client_body_reader;
    std::vector<uint8_t>                         header_line;
    std::vector<std::vector<uint8_t> >           request_body;
    std::vector<std::vector<uint8_t> >::iterator request_body_it;
    size_t                                       request_body_size;

    uint8_t _response_buf[4096];
    size_t  _response_buf_len;
    size_t  _response_buf_index;
    bool    _response_body_sent;
    bool    _send_header_and_abort;
};

#endif
