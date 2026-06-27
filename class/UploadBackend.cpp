#include "UploadBackend.hpp"
#include "webserv.hpp"

#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <strings.h>
#include <sys/types.h>

UploadBackend::UploadBackend(std::string const &root, HTTPStream &http_stream,
                             IRequestBody *request_body, ssize_t max_ressource_size)
    : _bytes_written(0), _to_write(0), root(root), http_stream(http_stream), _response_body(NULL),
      _request_body(request_body), _file_buf(), _max_ressource_size(max_ressource_size)
{
    bzero(_file_buf, 4096);
}

UploadBackend::~UploadBackend()
{
    delete _response_body;
    delete _request_body;
}

t_file_status UploadBackend::openFile(std::string const &URI)
{
    if (_ofile.is_open())
        return (_open);
    _ofile.open((root + URI).c_str(), std::ios::binary);
    if (!_ofile.is_open())
        return (_no_permission);
    if (_ofile.fail())
        return (_no_permission);
    char *end;
    _to_write = std::strtol(http_stream.request.peekHeader("Content-Length").c_str(), &end, 10);
    return (_open);
}

async::Future::Poll UploadBackend::writeFile()
{
    size_t s = 0;
    if (!_request_body)
        return (Ready);
    while (true)
    {
        container::Slice<uint8_t> sl(_file_buf, 4096);
        if (_request_body->poll_read(sl, s) == Pending)
            return (Pending);
        if (_request_body->fail() || !s)
            return (Ready);
        _ofile.write(reinterpret_cast<const std::ostream::char_type *>(_file_buf), s);
        if (_ofile.fail())
        {
            std::cerr << "Write failed. System error: " << std::strerror(errno) << std::endl;
            http_stream.setErrorResponse("500", "Internal Server Error");
            return (Ready);
        }
        _bytes_written += s;
    }
    return (Ready);
}

async::Future::Poll UploadBackend::setResponse() throw(async::OsError)
{
    if (http_stream.request.method == "POST")
    {
        char *end;
        size_t content_length = std::strtoul(http_stream.request.peekHeader("Content-Length").c_str(), &end, 10);
        (void)end;
        if (_max_ressource_size != -1 && content_length > static_cast<size_t>(_max_ressource_size))
        {
            http_stream.setErrorResponse("413", "Request Entity Too Large");
            return (Ready);
        }
        const t_file_status status = openFile(http_stream.request.URI);
        switch (status)
        {
        case _no_permission:
            http_stream.setErrorResponse("403", "Forbidden");
            break;
        case _open:
            if (writeFile() == Pending)
                return (Pending);
            http_stream.response.addHeader("Content-Length", "0");
            http_stream.setHTTPStatus("201", "Created");
            break;
        default:
            http_stream.setErrorResponse("500", "Internal Server Error");
        }
    }
    else
        return http_stream.setErrorResponse("405", "Method Not Allowed"), Ready;
    return Ready;
}

async::Future::Poll UploadBackend::poll() throw(async::OsError)
{
    if (this->setResponse() == Pending)
        return (Pending);
    if (http_stream.poll_send_headers(0, _response_body) == Pending)
        return (Pending);
    return (Ready);
}
