#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "FdStream.hpp"
#include "Request.hpp"
#include <async>
#include <container>
#include <string>
#include <sys/stat.h>
#include <vector>

typedef enum
{
    _open,
    _not_found,
    _no_permission,
    _already_open,
    _not_modified
} t_file_status;

typedef enum
{
    _first_step,
    _sending_head,
    _sending_body,
    _body_sent
} t_send_status;

class Response
{
  public:
    Response();
    Response(Response const &Response);
    ~Response();

    void               clear();
    void               headersAddBase();
    void               headAppend(std::string const &resp);
    void               bodyAppend(std::string const &resp);
    void               bodySet(std::string const &resp);
    void               addHeader(std::string const &key, std::string const &value);
    void               eraseHeader(std::string const &key);
    std::string const &getHTTPVersion() const;
    std::string const &getHTTPStatus() const;
    void               setHTTPVersion(std::string const &new_http_version);

    t_send_status send_status;
    std::string   head;
    std::string   body;

    std::multimap<std::string, std::string> headers;

  public:
    size_t               content_length;
    size_t               sent_bytes;
    std::string          http_version;
    std::string          http_status;
    std::string          http_msg;
    std::vector<uint8_t> send_buf;
    struct stat          sb;
};

#endif
