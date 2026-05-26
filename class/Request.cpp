#include "Request.hpp"
#include "html_generator.hpp"
#include "string_overload.hpp"
#include "webserv.h"

Request::Request(void)
    : _fd(-1), _first_line_read(false), _headers_read(false), _read_index(0),
      _write_index(0) {}

Request::Request(int fd)
    : _fd(fd), _first_line_read(false), _headers_read(false), _read_index(0),
      _write_index(0) {}

Request::Request(Request const &Request)
    : _fd(Request._fd), _first_line_read(Request._first_line_read),
      _headers_read(Request._headers_read), _read_index(0), _write_index(0),
      _method(Request._method), _URI(Request._URI),
      _http_version(Request._http_version), _headers(Request._headers) {}

Request::~Request(void) {}

Request const &Request::operator=(Request const &Request) {
    this->_method = Request._method;
    this->_URI = Request._URI;
    this->_http_version = Request._http_version;
    this->_fd = Request._fd;
    this->_first_line_read = Request._first_line_read;
    this->_headers_read = Request._headers_read;
    this->_headers = Request._headers;
    this->_read_index = Request._read_index;
    this->_write_index = Request._write_index;
    return (*this);
}

void Request::clear(int fd) {
    this->_method.clear();
    this->_URI.clear();
    this->_http_version.clear();
    this->_fd = fd;
    this->_first_line_read = false;
    this->_headers_read = false;
    this->_headers.clear();
    *this->_buffer = '\0';
    this->_read_index = 0;
    this->_write_index = 0;
}

void Request::prepareNextRead(void) {
    this->_method.clear();
    this->_URI.clear();
    this->_http_version.clear();
    this->_first_line_read = false;
    this->_headers_read = false;
    this->_headers.clear();
}

t_request_state Request::readRequest(void) {
    size_t read_bytes = read(this->_fd, this->_buffer + this->_write_index,
                             MAX_REQUEST_SIZE - this->_write_index);

    if (!read_bytes)
        return (_close);
    this->_buffer[read_bytes + this->_write_index] = '\0';
    this->_write_index += read_bytes;
    if (!_first_line_read)
        return (readFirstLine());
    return (readHeaders());
}

static size_t get_word(char *buffer, std::string &dest) {
    size_t len = 0;
    size_t skipped = 0;

    while (buffer[len] && std::isspace(buffer[len]))
        ++buffer, ++skipped;
    while (buffer[len] && !std::isspace(buffer[len]))
        ++len;
    dest.assign(buffer, len);
    return (len + skipped);
}

t_request_state Request::readFirstLine(void) {
    if (std::strchr(this->_buffer + this->_read_index, '\n')) {
        this->_read_index +=
            get_word(this->_buffer + this->_read_index, this->_method);
        this->_read_index +=
            get_word(this->_buffer + this->_read_index, this->_URI);
        this->_read_index +=
            get_word(this->_buffer + this->_read_index, this->_http_version);
        // if (!this->_URI.empty())
        //     this->_response = "HTTP/1.0 200 OK \r\n\r\ncoucou\r\n\r\n";
        this->_first_line_read = true;
        return (this->readHeaders());
    }
    return (_reading);
}

static size_t get_header(char *buffer,
                         std::map<std::string, std::string> &headers) {
    size_t whole_len = 0;
    size_t len = 0;
    std::pair<std::string, std::string> new_pair;

    while (*buffer && std::isspace(buffer[len])) {
        ++buffer;
        ++whole_len;
    }
    while (buffer[len] && !std::isspace(buffer[len++])) {
        if (buffer[len] == ':') {
            new_pair.first.assign(buffer, len++);
            break;
        }
    }
    if (new_pair.first.empty()) {
        return (whole_len);
    }
    buffer += len;
    whole_len += len;
    len = 0;
    while (buffer[len] && std::isspace(buffer[len])) {
        ++buffer;
        ++whole_len;
    }
    while (buffer[len] && buffer[len] != '\r' && buffer[len] != '\n')
        ++len;
    new_pair.second.assign(buffer, len);
    to_lower(new_pair.first);
    to_lower(new_pair.second);
    headers.insert(new_pair);
    return (whole_len + len);
}

t_request_state Request::readHeaders(void) {
    while (this->_buffer[this->_read_index] &&
           std::strcmp(this->_buffer + this->_read_index, "\r\n\r\n") &&
           std::strcmp(this->_buffer + this->_read_index, "\r\n\r\n") &&
           std::strstr(this->_buffer, "\n"))
        this->_read_index +=
            get_header(this->_buffer + this->_read_index, this->_headers);
    if (std::strstr(this->_buffer, "\r\n\r\n") ||
        std::strstr(this->_buffer, "\n\n"))
        return (_read);
    return (_reading);
}

/// @brief returns the next header available.
/// This function does not delete the returned header
/// (use getHeader for deletion).
/// @param key Key of the requested header, do not send to get the next header
/// @return value of the requested header or empty string if it does not exist
std::pair<std::string, std::string> Request::peekHeader(std::string const key) {
    std::pair<std::string, std::string> ret;
    std::map<std::string, std::string>::iterator it;

    if (key.empty())
        it = this->_headers.begin();
    else
        it = this->_headers.find(key);
    ret = *it;
    return (ret);
}

/// @brief returns the next header available.
/// This function deletes the returned header
/// (use peekHeader if this behavior is not desired).
/// @param key Key of the requested header, do not send to get the next header
/// @return value of the requested header or empty string if it does not exist
std::pair<std::string, std::string> Request::getHeader(std::string const key) {
    std::pair<std::string, std::string> ret = peekHeader(key);

    this->_headers.erase(key);
    return (ret);
}

std::string Request::getMethod(void) { return (this->_method); }

std::string Request::getURI(void) { return (this->_URI); }

std::string Request::getHTTPVersion(void) { return (this->_http_version); }

const char *Request::InvalidReadException::what() const throw() {
    return ("Error while reading");
}
