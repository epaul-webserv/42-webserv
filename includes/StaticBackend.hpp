#ifndef STATIC_BACKEND_HPP
#define STATIC_BACKEND_HPP

#include <async>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include "ABackend.hpp"
#include "HTTPStream.hpp"

class StaticBackend : public ABackend
{
public:
	StaticBackend(std::string const &root, HTTPStream &http_stream);
	~StaticBackend();

	Poll		setResponse() throw(async::OsError);
	Poll		setBodyWriter() throw(async::OsError);
	Poll		sendResponse() throw(async::OsError);
	Poll		poll() throw(async::OsError);
	std::string generateEtag() const;
	bool		conditionalGet(std::string const &etag);

private:
	static void	  setContentType(std::string const &file_ext, Response &response);
	t_file_status openFile(std::string const &path, Response &response);

	struct stat	   _sb;
	bool		   _file_ok;
	bool		   _response_set;
	size_t		   _body_written;
	std::string	   root;
	HTTPStream	  &http_stream;
	AResponseBody *_response_body;
	std::ifstream  _ifile;
};

#endif
