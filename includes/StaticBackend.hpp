#ifndef STATIC_BACKEND_HPP
#define STATIC_BACKEND_HPP

#include <async>
#include <string>
#include <sys/stat.h>
#include "ABackend.hpp"
#include "HTTPStream.hpp"

class StaticBackend : public ABackend
{
public:
	StaticBackend(std::string const &root, HTTPStream &http_stream);
	~StaticBackend(void);

	Poll setResponse() throw(async::OsError);
	Poll setBodyWriter() throw(async::OsError);
	Poll sendResponse() throw(async::OsError);
	Poll poll() throw(async::OsError);

private:
	void		  setContentType(std::string const &file_ext, Response &response);
	t_file_status openFile(std::string const &path, Response &response);

	struct stat	   _sb;
	bool		   _file_ok;
	bool		   _response_set;
	std::string	   root;
	HTTPStream	  &http_stream;
	AResponseBody *_response_body;
	FdStream	   _file_stream;
};

#endif
