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
                       socklen_t addr_len, Router &router)
    : addr(addr), addr_len(addr_len), _router(&router), _override_response(false), _rerun(false),
      _broken_connection(false), _keep_connection(false), _status(_fetching_request),
      _reactor(&reactor), _backend(NULL), _max_size(-1), _headers_status(_initializing_headers),
      _headers_sent_bytes(0), buf_io(reactor, fd)
{
}

HTTPStream::HTTPStream(HTTPStream const &other)
    : request(other.request), addr(other.addr), addr_len(other.addr_len), _router(other._router),
      _override_response(other._override_response), _rerun(other._rerun),
      _broken_connection(other._broken_connection), _keep_connection(other._keep_connection),
      _status(other._status), _reactor(other._reactor), _backend(other._backend),
      _max_size(other._max_size), _headers_status(other._headers_status),
      _headers_sent_bytes(other._headers_sent_bytes), buf_io(other.buf_io)
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
        setErrorResponse("400", "Bad Request");
        _status = _processing_response;
        return (Ready);
    }
    if (line.empty())
        return (Ready);
    get_next_word(line, request.method);
    if (request.method.empty())
        return (Ready);
    get_next_word(line, request.URI);
    get_next_word(line, request.http_version);
    _status = _fetching_headers;
    if (!decode_url(request.URI))
    {
        setErrorResponse("400", "Bad Request");
        std::cerr << "Invalid URI" << std::endl;
        return (Ready);
    }
    if (request.URI.find('?') != std::string::npos)
    {
        request.query_string = request.URI.substr(request.URI.find('?'));
        request.URI          = request.URI.substr(0, request.URI.find('?'));
    }
    request.rewrote_URI = request.URI;
    if (request.http_version.empty())
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
                request.headers[to_lower(_current_header.first)] = _current_header.second;
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
            request.headers[to_lower(_current_header.first)] = _current_header.second;
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

async::Future::Poll HTTPStream::keepAlive()
{
    this->response.clear();
    this->request.clear();
    this->status_pages.clear();
    this->_override_response = false;
    this->_rerun             = false;
    this->_saved_status.clear();
    this->_saved_msg.clear();
    this->_status = _fetching_request;
    delete this->_backend;
    this->_backend            = NULL;
    this->_headers_status     = _initializing_headers;
    this->_headers_sent_bytes = 0;
    return (this->poll());
}

void HTTPStream::headersAddBase()
{
    std::ostringstream head;

    if (request.http_version == "HTTP/0.9")
        return;
    head << request.http_version + " " + response.http_status + " " + response.http_msg << "\r\n";
    head << "Server: " << PROGNAME << "\r\n";
    head << "Date: " << Date().str() << "\r\n";
    if (response.content_length)
    {
        std::ostringstream tmp;
        tmp << response.content_length;
        response.headers["Content-Length"] = tmp.str();
    }
    std::map<std::string, std::string>::const_iterator ite = response.headers.end();
    for (std::map<std::string, std::string>::const_iterator it = response.headers.begin();
         it != ite; ++it)
        head << it->first << ": " << it->second << "\r\n";
    response.head.insert(0, head.str());
    response.head.append("\r\n");
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
    response.bodyAppend(get_html_error(response.http_status, response.http_msg));
    response.addHeader("Content-Type", "text/html");
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
        this->headersAddBase();
        if (!request.http_version.empty())
            response.send_buf = std::vector<uint8_t>(response.head.c_str(),
                                                     response.head.c_str() + response.head.size());
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
        if (this->buf_io.poll_flush_write() == Pending)
            return Pending;
    }
    return (Ready);
}

HTTPStream::~HTTPStream() { delete _backend; }

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
            if (this->sendHeaders() == Pending)
                return Pending;
        }
        try
        {
            if (_backend && _backend->poll() == Pending)
                return (Pending);
        }
        catch (std::runtime_error &e)
        {
            std::cerr << e.what() << std::endl;
            setErrorResponse("413", "Request Entity Too Large");
            if (this->sendHeaders() == Pending)
                return Pending;
            return (Ready);
        }
        catch (IRequestBody::BadReadException &e)
        {
            setErrorResponse("413", "Request Entity Too Large");
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
    if (!request.peekHeader("Content-Length").empty())
    {
        char  *end;
        size_t content_length = std::strtol(request.peekHeader("Content-Length").c_str(), &end, 10);

        return (new PlainRequestBody(buf_io, content_length));
    }
    request.headers["content-length"] = "";
    return (new ChunkedRequestBody(buf_io, 10,
                                   request.headers["content-length"])); // TODO: set max_size
}

async::Future::Poll HTTPStream::poll_send_headers(IResponseBody *&b)
{
    this->response.headers.erase("Content-Length");
    response.addHeader("Transfer-Encoding", "chunked");
    if (this->sendHeaders() == Pending)
        return (Pending);
    if (response.content_length)
        b = NULL;
    else
        b = new ChunkedResponseBody(buf_io);
    return async::Future::Ready;
}

async::Future::Poll HTTPStream::poll_send_headers(size_t content_length, IResponseBody *&b)
{
    std::ostringstream oss;

    this->response.headers.erase("Transfer-Encoding");
    if (content_length)
    {
        oss << content_length;
        this->response.addHeader("Content-Length", oss.str());
    }
    if (this->sendHeaders() == Pending)
        return (Pending);
    if (response.content_length || content_length == 0)
        b = NULL;
    else
        b = new PlainResponseBody(buf_io);
    return async::Future::Ready;
}
