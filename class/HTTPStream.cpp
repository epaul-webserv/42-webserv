#include "HTTPStream.hpp"
#include "string_overload.hpp"
#include "url_utils.hpp"
#include <iostream>

HTTPStream::HTTPStream(async::Reactor &reactor, int fd, struct sockaddr_storage addr,
                       socklen_t addr_len, Router *router)
    : addr(addr), addr_len(addr_len), buf_io(reactor, fd), _status(_fetching_request),
      _reactor(&reactor), _router(router), _backend(NULL)
{
    if (!this->_router)
        throw(std::runtime_error("Cannot initialize HTTPStream with NULL router"));
}

HTTPStream::HTTPStream(HTTPStream const &other)
    : addr(other.addr), addr_len(other.addr_len), request(other.request), buf_io(other.buf_io),
      _status(other._status), _reactor(other._reactor), _router(other._router),
      _backend(other._backend)
{
    (void)addr;
}

async::Future::Poll HTTPStream::readRequest()
{
    if (_status == _fetching_request)
        return (readFirstLine());
    return (readHeaders());
}

static size_t get_next_word(std::vector<uint8_t> &line, std::string &dest)
{
    size_t len     = 0;
    size_t skipped = 0;

    if (line.empty())
        return (0);
    while (!line.empty() && line[len] && std::isspace(line[len]))
        line.erase(line.begin()), ++skipped;
    if (line.empty())
        return (skipped);
    while (line[len] && !std::isspace(line[len]))
        ++len;
    dest = std::string(&line[0], &line[len]);
    line.erase(line.begin(), line.begin() + len);
    return (len + skipped);
}

static std::string get_next_word(std::vector<uint8_t> &line, char const &last_char)
{
    size_t      len     = 0;
    size_t      skipped = 0;
    std::string dest;

    if (line.empty())
        return (dest);
    while (!line.empty() && line[len] && std::isspace(line[len]))
        line.erase(line.begin()), ++skipped;
    if (line.empty())
        return (dest);
    while (line[len] && !std::isspace(line[len]) && (!len || line[len - 1] != last_char))
        ++len;
    dest = std::string(&line[0], &line[len]);
    line.erase(line.begin(), line.begin() + len);
    return (dest);
}

static std::string get_end_of_line(std::vector<uint8_t> &line)
{
    size_t      len     = 0;
    size_t      skipped = 0;
    std::string dest;

    if (line.empty())
        return (dest);
    while (line[len] && std::isspace(line[len]))
    {
        line.erase(line.begin()), ++skipped;
        if (line.empty())
            return (dest);
    }
    while (line[len] && line[len] != '\r' && line[len] != '\n')
        ++len;
    dest = std::string(&line[0], &line[len]);
    line.erase(line.begin(), line.begin() + len);
    return (dest);
}

async::Future::Poll HTTPStream::readFirstLine()
{
    std::vector<uint8_t> line;
    if (buf_io.poll_read_line(line) == Pending)
        return (Pending);
    if ((line.size() == 1 || line.size() == 2) && *(line.end() - 1) == '\n')
    {
        _status = _sending_response;
        return (Ready); // FIXME : need to set a 400 error
    }
    if (line.empty())
        return (Ready);
    get_next_word(line, request._method);
    if (request._method.empty())
        return (Ready);
    get_next_word(line, request._URI);
    get_next_word(line, request._http_version);
    _status = _fetching_headers;
    if (!decode_url(request._URI))
    {
        std::cerr << "Invalid URI" << std::endl; // FIXME : need to set a 400 error
        return (Ready);
    }
    if (request._URI.find('?') != std::string::npos)
    {
        request._query_string = request._URI.substr(request._URI.find('?'));
        request._URI          = request._URI.substr(0, request._URI.find('?'));
    }
    if (request._http_version.empty())
        return (Ready);
    return (this->readHeaders());
}

async::Future::Poll HTTPStream::readHeaders()
{
    std::vector<uint8_t> line;

    while (true)
    {
        if (buf_io.poll_read_line(line) == Pending)
            return (Pending);
        if (line.empty())
            break;
        std::vector<uint8_t>::iterator ite = line.end();
        if ((line.size() == 2 && line[0] == '\r') || (line.size() == 1 && line[0] == '\n'))
        {
            if (!_current_header.first.empty())
                request._headers[to_lower(_current_header.first)] = _current_header.second;
            break;
        }
        if (line[0] == ' ' || line[0] == '\t')
        {
            _current_header.second.append(get_end_of_line(line));
            line.clear();
            continue;
        }
        if (!_current_header.first.empty())
        {
            request._headers[to_lower(_current_header.first)] = _current_header.second;
            _current_header.first.clear();
            _current_header.second.clear();
        }
        std::vector<uint8_t>::iterator it = std::find(line.begin(), line.end(), ':');
        _current_header.first             = get_next_word(line, ':');
        if (_current_header.first.find(':') == std::string::npos)
        {
            _current_header.first.clear();
            line.clear();
            continue;
        }
        _current_header.first = _current_header.first.substr(0, _current_header.first.find(':'));
        if (it != ite)
            _current_header.second = get_end_of_line(line);
        line.clear();
    }
    return (Ready);
}

HTTPStream::~HTTPStream() {}

HTTPStream &HTTPStream::operator=(HTTPStream const &other)
{
    if (this != &other)
    {
        addr     = other.addr;
        addr_len = other.addr_len;
        buf_io   = other.buf_io;
        _status  = other._status;
        _reactor = other._reactor;
        request  = other.request;
    }
    return (*this);
}

async::Future::Poll HTTPStream::poll() throw(async::OsError)
{
    if (this->_status <= _fetching_headers)
    {
        if (readRequest() == Pending)
            return (Pending);
        // std::cout << this->_request;
        if (this->_status != _sending_response)
            this->_status = _getting_backend;
    }
    if (this->_status == _getting_backend)
        this->_backend = this->_router->resolve(*_reactor, *this, false);
    // FIXME: handle body and response here
    return (Ready);
}
