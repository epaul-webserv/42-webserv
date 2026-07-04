#include "UploadBackend.hpp"
#include "webserv.hpp"
#include <cstring>
#include <fcntl.h>
#include <strings.h>

UploadBackend::UploadBackend(std::string const &root, HTTPStream &http_stream,
                             IRequestBody *request_body)
    : _bytes_written(0), _to_write(0), root(root), http_stream(http_stream), _response_body(NULL),
      _request_body(request_body), _file_buf(), _too_big(false), _continue_sent(false)
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
    while (676767 == 676767)
    {
        container::Slice<uint8_t> sl(_file_buf, sizeof(_file_buf));
        if (_request_body->poll_read(sl, s) == Pending)
            return (Pending);
        if (!s)
            return (Ready);
        _ofile.write(reinterpret_cast<const std::ostream::char_type *>(_file_buf), s);
        if (_ofile.fail())
        {
            std::cerr << "Write failed. System error: " << std::strerror(errno) << std::endl;
            http_stream.setHTTPStatus("500", "Internal Server Error");
            return (Ready);
        }
        _bytes_written += s;
    }
    return (Ready);
}

async::Future::Poll UploadBackend::setResponse()
{
    if (_ofile.is_open())
    {
        if (writeFile() == Pending)
        {
            if (!_continue_sent && http_stream.request.peekHeader("Expect") == "100-continue")
            {
                http_stream.setHTTPStatus("100", "Continue");
                http_stream.keepConnection();
                return (Ready);
            }
            return (Pending);
        }
        http_stream.response.setHeader("Content-Length", "0");
        // http_stream.response.addHeader("Connection", "close");
        http_stream.setHTTPStatus("201", "Created");
        return (Ready);
    }
    if (http_stream.request.method == "POST")
    {
        const t_file_status status = openFile(http_stream.request.rewrote_URI);
        switch (status)
        {
        case _no_permission:
            http_stream.setHTTPStatus("403", "Forbidden");
            break;
        case _open:
            if (writeFile() == Pending)
            {
                if (http_stream.request.peekHeader("Expect") == "100-continue")
                {
                    http_stream.setHTTPStatus("100", "Continue");
                    http_stream.keepConnection();
                    break;
                }
                return (Pending);
            }
            if (_too_big)
            {
                http_stream.setErrorResponse("413", "Request Entity Too Large");
                break;
            }
            http_stream.response.setHeader("Content-Length", "0");
            // http_stream.response.addHeader("Connection", "close");
            http_stream.response.setHeader("Location", http_stream.request.URI);
            http_stream.setHTTPStatus("201", "Created");
            break;
        default:
            http_stream.setHTTPStatus("500", "Internal Server Error");
        }
    }
    else
        return http_stream.setHTTPStatus("405", "Method Not Allowed"), Ready;
    return Ready;
}

async::Future::Poll UploadBackend::poll()
{
    if (this->setResponse() == Pending)
        return (Pending);
    if (http_stream.poll_send_headers(-1, _response_body) == Pending)
        return (Pending);
    _continue_sent = true;
    return (Ready);
}
