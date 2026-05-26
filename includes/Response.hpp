#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "Request.hpp"
#include <string>
#include <sys/stat.h>

typedef enum { _open, _not_found, _no_permission, _already_open } t_file_status;

typedef enum { _first_step, _head_sent, _body_sent } t_send_status;

typedef enum { _init, __pending, _ready } t_fetch_status;

class Response {
public:
    Response(int fd);
    ~Response(void);

    void set(int fd);
    bool sendResponse(void);
    bool ready(void);
    void headAppend(std::string const &resp);
    void bodyAppend(std::string const &resp);
    t_file_status openFile(std::string const &path);
    void setHttpVersion(std::string const &http_version);

private:
    Response(void);
    Response(Response const &Response);

    t_send_status _send_status;
    t_fetch_status _fetch_status;
    int _conn_fd;
    int _file_fd;
    char _head[8192];
    char *_body;
    size_t _header_read_index;
    size_t _header_write_index;
    size_t _body_read_index;
    size_t _body_write_index;
    size_t _content_length;
    size_t _sent_bytes;
    std::string _http_version;
    std::string _http_status;
    struct stat _sb;
    void *_map;
};

#endif
