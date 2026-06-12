#include "HTTPStream.hpp"
#include "string_overload.hpp"
#include "url_utils.hpp"
#include <iostream>

HTTPStream::HTTPStream(async::Reactor &reactor, int fd, struct sockaddr_storage addr,
                       socklen_t addr_len)
    : addr(addr), addr_len(addr_len), io(reactor, fd), buf_io(io), _status(_fetching_request),
      _reactor(&reactor), _request_first_line_read(false)
{
    (void)addr;
}

HTTPStream::HTTPStream(HTTPStream const &HTTPStream)
    : addr(HTTPStream.addr), addr_len(HTTPStream.addr_len), buf(HTTPStream.buf), io(HTTPStream.io),
      buf_io(HTTPStream.buf_io), _status(HTTPStream._status), _reactor(HTTPStream._reactor),
      _request(HTTPStream._request), _request_first_line_read(false)
{
    (void)addr;
}

async::Future::Poll HTTPStream::readRequest()
{
    if (!this->_request_first_line_read)
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

static std::string get_next_word(std::vector<uint8_t> &line)
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
    while (line[len] && !std::isspace(line[len]))
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
        line.erase(line.begin()), ++skipped;
    while (line[len] && line[len] != '\r' && line[len] != '\n')
        ++len;
    dest = std::string(&line[0], &line[len]);
    line.erase(line.begin(), line.begin() + len);
    return (dest);
}

async::Future::Poll HTTPStream::readFirstLine()
{
    std::vector<uint8_t> line;

    if (buf_io.poll_read_line(line) == async::Future::Pending)
        return (async::Future::Pending);
    if (line.empty())
        return (async::Future::Ready);
    get_next_word(line, _request._method);
    if (_request._method.empty())
        return (async::Future::Ready);
    get_next_word(line, _request._URI);
    get_next_word(line, _request._http_version);
    this->_request_first_line_read = true;
    decode_url(_request._URI);
    if (_request._URI.find('?') != std::string::npos)
    {
        _request._query_string = _request._URI.substr(_request._URI.find('?'));
        _request._URI          = _request._URI.substr(0, _request._URI.find('?'));
    }
    if (_request._http_version.empty())
        return (async::Future::Ready);
    return (this->readHeaders());
}

async::Future::Poll HTTPStream::readHeaders()
{
    std::vector<uint8_t> line;

    while (true)
    {
        if (buf_io.poll_read_line(line) == async::Future::Pending)
            return (async::Future::Pending);
        if (line.empty())
            break;
        std::vector<uint8_t>::iterator ite = line.end();
        if ((line.size() == 2 && line[0] == '\r') || (line.size() == 1 && line[0] == '\n'))
            break;
        std::vector<uint8_t>::iterator it         = std::find(line.begin(), line.end(), ':');
        std::string                    header_key = get_next_word(line);
        if (header_key.find(':') == std::string::npos)
        {
            _request._headers.insert((std::pair<std::string, std::string>(header_key, "")));
            line.clear();
            break;
        }
        header_key = header_key.substr(0, header_key.find(':'));
        if (it != ite)
            _request._headers.insert(
                (std::pair<std::string, std::string>(to_lower(header_key), get_end_of_line(line))));
        line.clear();
    }
    return (async::Future::Ready);
}

HTTPStream::~HTTPStream() {}

HTTPStream &HTTPStream::operator=(HTTPStream const &other)
{
    if (this != &other)
    {
        addr     = other.addr;
        addr_len = other.addr_len;
        buf      = other.buf;
        io       = other.io;
        buf_io   = other.buf_io;
        _status  = other._status;
        _reactor = other._reactor;
        _request = other._request;
    }
    return (*this);
}

async::Future::Poll HTTPStream::poll() throw(async::OsError)
{
    if (this->_status == _fetching_request)
    {
        if (readRequest() == Future::Pending)
            return (Future::Pending);
        std::cout << this->_request;
    }
    return (Future::Ready);
}
