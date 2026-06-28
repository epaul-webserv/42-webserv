#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "webserv.hpp"
#include <algorithm>
#include <async>
#include <map>
#include <sstream>
#include <string>

typedef enum
{
    _reading,
    _read,
    _close
} t_request_state;

typedef enum
{
    _dir,
    _file,
    _no_access,
    _none,
    _err
} t_file_type;

class Request
{
  public:
    Request();
    Request(const Request &Request);
    ~Request();

    Request                            &operator=(const Request &other);
    void                                clear();
    void                                prepareNextRead();
    std::string                         peekHeader(std::string const &key = "") const;
    std::pair<std::string, std::string> takeHeader(std::string const &key = "");

    class InvalidReadException : public std::exception
    {
        const char *what() const throw();
    };

    std::string                        method;
    std::string                        URI;
    std::string                        rewrote_URI;
    std::string                        query_string;
    std::string                        http_version;
    std::map<std::string, std::string> headers;
};

std::ostream &operator<<(std::ostream &o, Request &request);

#endif
