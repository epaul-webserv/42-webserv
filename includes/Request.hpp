#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "FdStream.hpp"
#include "default_conf.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <map>
#include <sstream>
#include <string>

typedef enum { _reading, _read, _close } t_request_state;

typedef enum { _dir, _file, _no_access, _none, _err } t_file_type;

class Request {
  public:
    Request(void);
    Request(const Request &Request);
    Request(int fd);
    ~Request(void);

    Request const &operator=(const Request &Request);
    void clear(int fd);
    void prepareNextRead(void);
    t_request_state readRequest(void);
    bool sendResponse(void);
    std::pair<std::string, std::string> peekHeader(std::string const key = "");
    std::pair<std::string, std::string> getHeader(std::string const key = "");
    std::string getMethod(void);
    std::string getURI(void);
    std::string getHTTPVersion(void);
    void prepareResponse();

    class InvalidReadException : public std::exception {
        const char *what(void) const throw();
    };

  private:
    t_request_state readFirstLine(void);
    t_request_state readHeaders(void);
    std::string getFirstLine(void) const;
    void response_0_9(void);
    void response_1_0(void);
    void response_1_1(void);
    bool sendFile(void);
    void createErrorResponse(std::string const &errorCode,
                             std::string const &errorMessage);

    int _fd;
    bool _first_line_read;
    bool _headers_read;
    char _buffer[MAX_REQUEST_SIZE + 1];
    size_t _read_index;
    size_t _write_index;
    std::string _method;
    std::string _URI;
    std::string _http_version;
    std::map<std::string, std::string> _headers;
    // StaticResponse                     _response;
};

#endif
