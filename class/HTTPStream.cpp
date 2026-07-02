#include "HTTPStream.hpp"
#include "ChunkedRequestBody.hpp"
#include "ChunkedResponseBody.hpp"
#include "PlainRequestBody.hpp"
#include "PlainResponseBody.hpp"
#include "html_generator.hpp"
#include "string_overload.hpp"
#include "url_utils.hpp"
#include <iostream>

HTTPStream::HTTPStream(async::Reactor &reactor, int fd, const struct sockaddr_storage &addr,
                       socklen_t addr_len, Router &router, std::string host_str)
    : addr(addr), addr_len(addr_len), _router(&router), _host_str(host_str), _max_resource_size(-1),
      _override_response(false), _rerun(false), _broken_connection(false), _keep_connection(false),
      _body_read(false), _status(_fetching_request), _reactor(&reactor), _backend(NULL),
      _headers_status(_initializing_headers), _headers_sent_bytes(0), buf_io(reactor, fd),
      _request_body(NULL)
{
}

HTTPStream::~HTTPStream() { delete _backend; }

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
    if (buf_io.poll_read_line(_header_line) == Pending)
        return (Pending);
    if ((_header_line.size() == 1 || _header_line.size() == 2) && *(_header_line.end() - 1) == '\n')
    {
        request.http_version = "HTTP/0.9";
        setErrorResponse("400", "Bad Request");
        _status = _processing_response;
        return (Ready);
    }
    if (_header_line.empty())
    {
        request.http_version = "HTTP/0.9";
        setBrokenConnection();
        return (Ready);
    }
    get_next_word(_header_line, request.method);
    if (request.method.empty())
        return (Ready);
    get_next_word(_header_line, request.URI);
    get_next_word(_header_line, request.http_version);
    _status = _fetching_headers;
    if (!decode_url(request.URI))
    {
        setErrorResponse("400", "Bad Request");
        std::cerr << "Invalid URI" << std::endl;
        return (Ready);
    }
    if (request.URI.find('?') != std::string::npos)
    {
        request.query_string = request.URI.substr(request.URI.find('?') + 1);
        request.URI          = request.URI.substr(0, request.URI.find('?'));
    }
    request.rewrote_URI = request.URI;
    if (request.http_version.empty())
    {
        request.http_version = "HTTP/0.9";
        return (Ready);
    }
    if ((_header_line.size() == 2 && (_header_line[0] == '\r' || _header_line[1] == '\n')) ||
        (_header_line.size() == 1 && _header_line[0] == '\n'))
        _header_line.clear();
    else
    {
        setBrokenConnection();
        setErrorResponse("400", "Bad Request");
        std::cerr << "Invalid URI" << std::endl;
        return (Ready);
    }
    return (this->readHeaders());
}

async::Future::Poll HTTPStream::readHeaders()
{
    while (true)
    {
        if (buf_io.poll_read_line(_header_line) == Pending)
            return (Pending);
        if (_header_line.empty())
            break;
        std::vector<uint8_t>::iterator ite = _header_line.end();
        if ((_header_line.size() == 2 && _header_line[0] == '\r') ||
            (_header_line.size() == 1 && _header_line[0] == '\n'))
        {
            _header_line.clear();
            if (!_current_header.first.empty())
                request.headers[to_lower(_current_header.first)] = _current_header.second;
            break;
        }
        if (_header_line[0] == ' ' || _header_line[0] == '\t')
        {
            _current_header.second.append(get_end_of_line(_header_line));
            _header_line.clear();
            continue;
        }
        if (!_current_header.first.empty())
        {
            request.headers[to_lower(_current_header.first)] = _current_header.second;
            _current_header.first.clear();
            _current_header.second.clear();
        }
        std::vector<uint8_t>::iterator it =
            std::find(_header_line.begin(), _header_line.end(), ':');
        _current_header.first = get_next_word(_header_line, ':');
        if (_current_header.first.find(':') == std::string::npos)
        {
            _current_header.first.clear();
            _header_line.clear();
            continue;
        }
        _current_header.first = _current_header.first.substr(0, _current_header.first.find(':'));
        if (it != ite)
            _current_header.second = get_end_of_line(_header_line);
        _header_line.clear();
    }
    return (Ready);
}

async::Future::Poll HTTPStream::consume_body()
{
    uint8_t                   buf[4096];
    container::Slice<uint8_t> sl(buf, 4096);
    size_t                    s = 1;

    request_body();
    if (!_request_body)
        return (Ready);
    while (s)
    {
        try
        {
            if (_request_body->poll_read(sl, s) == Pending)
                return (Pending);
        }
        catch (std::exception &e)
        {
            std::cerr << "Error while reading body : " << e.what() << std::endl;
            _body_read = true;
            return (Ready);
        }
    }
    delete _request_body;
    _request_body = NULL;
    _body_read    = true;
    return (Ready);
}

async::Future::Poll HTTPStream::keepAlive()
{
    this->response.clear();
    this->request.clear();
    this->status_pages.clear();
    this->_max_resource_size = -1;
    this->_override_response = false;
    this->_rerun             = false;
    this->_broken_connection = false;
    this->_keep_connection   = false;
    this->_body_read         = false;
    this->_saved_status.clear();
    this->_saved_msg.clear();
    this->_status = _fetching_request;
    delete this->_backend;
    this->_backend        = NULL;
    this->_headers_status = _initializing_headers;
    this->_header_line.clear();
    this->_headers_sent_bytes = 0;
    this->_current_header.first.clear();
    this->_current_header.second.clear();
    this->_request_body = NULL;
    return (this->poll());
}

/// @brief Sets the response's status and sets the body if an associated file is set in the
/// configuration or in case of error (3XX, 4XX, 5XX)
void HTTPStream::setHTTPStatus(std::string const &status, std::string const &message)
{
    response.http_status = status;
    response.http_msg    = message;
}

void HTTPStream::setErrorResponse(std::string const &status, std::string const &message)
{
    setHTTPStatus(status, message);
    response.bodySet(get_html_error(response.http_status, response.http_msg));
    response.addHeader("content-type", "text/html");
}

bool HTTPStream::setStatusResponse()
{
    if (response.http_status.empty())
        setErrorResponse("500", "Internal Server Error");
    const std::map<std::string, std::string>::const_iterator it =
        status_pages.find(response.http_status);
    if (it != status_pages.end())
    {
        request.URI         = it->second;
        request.rewrote_URI = it->second;
        request.method      = "GET";
        _saved_status       = response.http_status;
        _saved_msg          = response.http_msg;
        response.clear();
        delete _backend;
        _backend           = _router->resolve(*_reactor, *this, true);
        _override_response = true;
        _rerun             = true;
        return (true);
    }
    return (false);
}

void HTTPStream::setBrokenConnection() { _broken_connection = true; }

void HTTPStream::keepConnection() { _keep_connection = true; }

async::Future::Poll HTTPStream::sendHeaders() throw(async::OsError)
{
    if (this->_headers_status == _initializing_headers)
    {
        if (!_override_response && setStatusResponse())
            return (Ready);
        if (_override_response)
        {
            if (response.http_status.empty() || response.http_status[0] == '4' ||
                response.http_status[0] == '5')
                setErrorResponse(_saved_status, _saved_msg);
            setHTTPStatus(_saved_status, _saved_msg);
        }
        this->response.headersAddBase();
        if (request.http_version != "HTTP/0.9")
            response.send_buf = std::vector<uint8_t>(response.head.c_str(),
                                                     response.head.c_str() + response.head.size());
        if (request.method != "HEAD")
            response.send_buf.insert(response.send_buf.end(), response.body.begin(),
                                     response.body.end());
        std::cerr << request.method << " " << request.URI << " " << response.getHTTPVersion()
                  << " - " << response.getHTTPStatus() << std::endl;
        this->_headers_status = _sending_headers;
    }
    if (this->_headers_status == _sending_headers)
    {
        while (_headers_sent_bytes < response.send_buf.size())
        {
            size_t s;
            if (this->buf_io.poll_write(
                    container::Slice<uint8_t>(&response.send_buf[_headers_sent_bytes],
                                              response.send_buf.size() - _headers_sent_bytes),
                    s) == async::Future::Pending)
                return (Pending);
            _headers_sent_bytes += s;
        }
        if (this->buf_io.poll_flush() == Pending)
            return Pending;
    }
    return (Ready);
}
//
// HTTPStream &HTTPStream::operator=(HTTPStream const &other)
// {
//     if (this != &other)
//     {
//         addr     = other.addr;
//         addr_len = other.addr_len;
//         buf_io   = other.buf_io;
//         _status  = other._status;
//         _reactor = other._reactor;
//         request  = other.request;
//     }
//     return (*this);
// }

async::Future::Poll HTTPStream::poll() throw(async::OsError)
{
    if (this->_status <= _fetching_headers)
    {
        if (readRequest() == Pending)
            return (Pending);
        if (this->_broken_connection)
            return (Ready);
        if (this->_status != _processing_response)
            this->_status = _getting_backend;
    }
    if (this->_status == _getting_backend)
    {
        this->_backend = this->_router->resolve(*_reactor, *this, false);
        this->_status  = _processing_response;
    }
    if (this->_status == _processing_response)
    {
        if (_backend == NULL)
        {
            if (!_body_read && consume_body() == Pending)
                return Pending;
            if (this->sendHeaders() == Pending)
                return Pending;
        }
        try
        {
            if (_backend && _backend->poll() == Pending)
                return (Pending);
        }
        catch (IRequestBody::TooLargeException &e)
        {
            std::cerr << e.what() << std::endl;
            setErrorResponse("413", "Request Entity Too Large");
            _body_read = true;
            setBrokenConnection();
            delete this->_backend;
            this->_backend = NULL;
            if (this->sendHeaders() == Pending)
                return Pending;
            return (Ready);
        }
        catch (IRequestBody::BadReadException &e)
        {
            setErrorResponse("413", "Bad body");
            if (this->sendHeaders() == Pending)
                return Pending;
            return (Ready);
        }
        if (_rerun)
        {
            _rerun = false;
            if (_backend && _backend->poll() == Pending)
                return (Pending);
        }
        if (this->_keep_connection)
        {
            this->_headers_status = _initializing_headers;
            this->response.clear();
            this->_headers_sent_bytes = 0;
            this->_keep_connection    = false;
            return (Future::Pending);
        }
        if (!this->_broken_connection && this->request.http_version == "HTTP/1.1" &&
            this->response.headers["Connection"] != "close")
            return (this->keepAlive(), Future::Pending);
        return (Future::Ready);
    }
    return (Ready);
}

IRequestBody *HTTPStream::request_body()
{
    if (_request_body)
        return (_request_body);
    if (request.method == "HEAD")
        return (NULL);
    if (!request.peekHeader("Content-Length").empty())
    {
        char  *end;
        size_t content_length = std::strtol(request.peekHeader("Content-Length").c_str(), &end, 10);
        _request_body         = new PlainRequestBody(buf_io, content_length);
        return (_request_body);
    }
    request.headers["content-length"] = "";
    if (request.peekHeader("Transfer-Encoding") == "chunked")
    {
        _request_body =
            new ChunkedRequestBody(buf_io, _max_resource_size, request.headers["content-length"]);
        return (_request_body);
    }
    return (NULL);
}

async::Future::Poll HTTPStream::poll_send_headers(IResponseBody *&b)
{
    this->response.eraseHeader("Content-Length");
    response.addHeader("transfer-encoding", "chunked");
    if (this->sendHeaders() == Pending)
        return (Pending);
    if (response.content_length)
        b = NULL;
    else
        b = new ChunkedResponseBody(buf_io);
    return async::Future::Ready;
}

async::Future::Poll HTTPStream::poll_send_headers(ssize_t content_length, IResponseBody *&b)
{
    std::ostringstream oss;

    this->response.eraseHeader("transfer-encoding");
    if (content_length != -1)
    {
        oss << content_length;
        this->response.addHeader("Content-Length", oss.str());
    }
    if (this->sendHeaders() == Pending)
        return (Pending);
    if (response.content_length || content_length <= 0)
        b = NULL;
    else
        b = new PlainResponseBody(buf_io);
    return async::Future::Ready;
}
