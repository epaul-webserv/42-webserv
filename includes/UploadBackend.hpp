#ifndef UPLOAD_BACKEND_HPP
#define UPLOAD_BACKEND_HPP

#include "ABackend.hpp"
#include "HTTPStream.hpp"
#include "IRequestBody.hpp"
#include "IResponseBody.hpp"
#include <async>
#include <fstream>
#include <string>

class UploadBackend : public ABackend
{
  public:
    UploadBackend(std::string const &root, HTTPStream &http_stream, IRequestBody *request_body);
    ~UploadBackend();
    t_file_status       openFile(std::string const &URI);
    async::Future::Poll writeFile();

    Poll setResponse() throw(async::OsError, std::exception);
    Poll poll() throw(async::OsError, std::exception);

  private:
    UploadBackend();

    size_t         _bytes_written;
    size_t         _to_write;
    std::string    root;
    HTTPStream    &http_stream;
    IResponseBody *_response_body;
    IRequestBody  *_request_body;
    std::ofstream  _ofile;
    uint8_t        _file_buf[65535];
    bool           _too_big;
    bool           _continue_sent;
};

#endif
