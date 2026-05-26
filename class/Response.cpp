#include "Response.hpp"
#include "webserv.h"
#include <cstring>
#include <fcntl.h>
#include <strings.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdexcept>

Response::Response(int fd)
    : _send_status(_first_step), _fetch_status(_init),
      _conn_fd(fd), _file_fd(-1),
      _body(NULL),
      _header_read_index(0), _header_write_index(0), _body_read_index(0),
      _body_write_index(0), _content_length(0), _sent_bytes(0), _map(NULL) {
    bzero(&this->_sb, sizeof(this->_sb));
}

Response::~Response(void) {
    if (this->_map)
        munmap(this->_map, this->_sb.st_size);
    if (this->_file_fd != -1)
        close(this->_file_fd);
    if (this->_body)
        delete[] this->_body;
}

void Response::set(int fd) {
    this->_conn_fd = fd;
    if (this->_file_fd != -1)
        close(this->_file_fd);
    if (this->_map)
        munmap(this->_map, this->_sb.st_size);
    this->_send_status = _first_step;
    this->_map = NULL;
    this->_header_read_index = 0;
    this->_header_write_index = 0;
    this->_body_read_index = 0;
    this->_body_write_index = 0;
    this->_sent_bytes = 0;
    this->_content_length = 0;
    bzero(&this->_sb, sizeof(this->_sb));
}

bool Response::ready(void) {
    if (this->_fetch_status == _ready)
        return (true);
    return (false);
}

void Response::headAppend(std::string const &resp) {
    const char *c_resp = resp.c_str();

    std::memcpy(this->_head + this->_header_write_index, c_resp, resp.size());
    this->_header_write_index += resp.size();
}

void Response::bodyAppend(std::string const &resp) {
    char *new_body = new char[sizeof(this->_body) + resp.size()];

    if (this->_body) {
        std::memcpy(new_body, this->_body, this->_body_write_index);
        delete[] this->_body;
    }
    this->_body = new_body;
    std::memcpy(this->_body + this->_body_write_index, resp.c_str(),
                resp.size());
    this->_content_length += resp.size();
    this->_body_write_index += resp.size();
}

t_file_status Response::openFile(std::string const &path) {
    if (this->_map)
        return (_already_open);
    if (stat((path).c_str(), &this->_sb) == -1)
        return (_not_found);
    this->_file_fd = open((path).c_str(), O_RDONLY);
    if (this->_file_fd < 0) {
        if (errno == EACCES)
            return (_no_permission);
        return (_no_permission);
    }
    // if (!this->_sb.st_size)
    //     return (_open);
    this->_map = mmap(NULL, this->_sb.st_size, PROT_READ, MAP_PRIVATE,
                      this->_file_fd, 0);
    if (this->_map == MAP_FAILED) {
        close(this->_file_fd);
        this->_file_fd = -1;
        this->_map = NULL;
        return (_not_found);
    }
    return (_open);
}

void Response::setHttpVersion(std::string const &http_version) {
    if (http_version != "HTTP/1.0" && http_version != "HTTP/1.1") {
        throw (std::runtime_error("Invalid HTTP version"));
    }
    this->_http_version = http_version;
}
