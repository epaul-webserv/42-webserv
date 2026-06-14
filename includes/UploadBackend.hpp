#ifndef UPLOAD_BACKEND_HPP
#define UPLOAD_BACKEND_HPP

#include <async>
#include <fstream>
#include <string>
#include "ABackend.hpp"
#include "HTTPStream.hpp"

class UploadBackend : public ABackend
{
public:
	UploadBackend(std::string const &root, HTTPStream &http_stream, ARequestBody &request_body);
	~UploadBackend();
	t_file_status		openFile(std::string const &URI);
	async::Future::Poll writeFile();

	Poll setResponse() throw(async::OsError);
	Poll poll() throw(async::OsError);

private:
	UploadBackend();

	size_t		   _bytes_written;
	size_t		   _to_write;
	std::string	   root;
	HTTPStream	  &http_stream;
	AResponseBody *_response_body;
	ARequestBody  &_request_body;
	std::ofstream  _ofile;
	uint8_t		   _file_buf[4096];
};

#endif
