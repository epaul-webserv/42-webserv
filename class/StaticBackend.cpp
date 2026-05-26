#include "StaticBackend.hpp"
#include "webserv.h"
#include <cstring>
#include <fcntl.h>
#include <strings.h>
#include <sys/mman.h>
#include <unistd.h>

StaticBackend::~StaticBackend(void) {
    if (this->_map)
        munmap(this->_map, this->_sb.st_size);
}

void StaticBackend::clear(int fd) {
    this->_conn_fd = fd;
    if (this->_file_fd != -1)
        close(this->_file_fd);
    if (this->_map)
        munmap(this->_map, this->_sb.st_size);
    this->_status = _init;
    this->_map = NULL;
    this->_read_index = 0;
    this->_write_index = 0;
    this->_body_read_index = 0;
    this->_body_write_index = 0;
    this->_sent_bytes = 0;
    this->_content_length = 0;
    bzero(&this->_sb, sizeof(this->_sb));
}

void StaticBackend::prepareNext(void) {
    this->_status = _init;
    if (this->_file_fd != -1)
        close(this->_file_fd);
    if (this->_map)
        munmap(this->_map, this->_sb.st_size);
    this->_body[0] = '\0';
    this->_map = NULL;
    this->_read_index = 0;
    this->_write_index = 0;
    this->_body_read_index = 0;
    this->_body_write_index = 0;
    this->_content_length = 0;
    this->_sent_bytes = 0;
    bzero(&this->_sb, sizeof(this->_sb));
}

void StaticBackend::headAppend(std::string const &resp) {
    const char *c_resp = resp.c_str();

    std::memcpy(this->_head + this->_write_index, c_resp, resp.size());
    this->_write_index += resp.size();
}

void StaticBackend::bodyAppend(std::string const &resp) {
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

t_file_status StaticBackend::openFile(std::string path) {
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

void StaticBackend::setHeaders() {
    std::ostringstream headers;

    headers << "Server: " << PROGNAME << "\r\n";
    headers << "Date: " << Date().str() << "\r\n";
    headers << "Content-Type: text/html\r\n";
    if (this->_map || this->_content_length)
        headers << "Content-Length: "
            << (this->_map ? this->_sb.st_size : this->_content_length)
            << "\r\n";
    headers << "Connection: close" << "\r\n";
    headers << "\r\n";
    this->headAppend(headers.str());
}

void StaticBackend::sendHead(void) {
    int sent_bytes = send(this->_conn_fd, this->_head + this->_read_index,
                          this->_write_index - this->_read_index, 0);

    if (sent_bytes == -1)
        return;
    this->_read_index += sent_bytes;
    if (this->_read_index == this->_write_index)
        this->_status = _head_sent;
}

bool StaticBackend::sendBody(void) {
    char *buffer;
    size_t to_read;

    if (this->_map) {
        buffer = (char *)this->_map;
        to_read = this->_sb.st_size;
    } else if (this->_body) {
        buffer = this->_body;
        to_read = _body_write_index;
    } else
        return (this->_status = _body_sent);
    int sent_bytes = send(this->_conn_fd, buffer + this->_sent_bytes,
                          to_read - this->_sent_bytes, MSG_NOSIGNAL);

    if (sent_bytes == -1)
        return (true);
    this->_sent_bytes += sent_bytes;
    if (this->_sent_bytes == to_read) {
        if (this->_map) {
            munmap(this->_map, this->_sb.st_size);
            this->_map = NULL;
            close(this->_file_fd);
            this->_file_fd = -1;
        }
        if (this->_body) {
            delete[] this->_body;
            this->_body = NULL;
        }
        this->_status = _body_sent;
        return (true);
    }
    return (false);
}

bool StaticBackend::sendStaticBackend(void) {
    if (this->_status == _init)
        return (this->sendHead(), false);
    if (this->_status == _head_sent)
        return (this->sendBody());
    return (true);
}

static t_file_type fileType(std::string const &path, std::string const &root) {
    struct stat s = {};

    if (stat((root + path).c_str(), &s) == -1) {
        if (errno == EACCES)
            return (_no_access);
        if (errno == ENOMEM)
            return (_err);
        return (_none);
    }
    switch (s.st_mode & S_IFMT) {
    case S_IFDIR:
        return (_dir);
    case S_IFREG:
        return (_file);
    default:
        return (_none);
    }
}
