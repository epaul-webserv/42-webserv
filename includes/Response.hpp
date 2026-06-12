#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <async>
#include <container>
#include <string>
#include <sys/stat.h>
#include <vector>
#include "FdStream.hpp"
#include "Request.hpp"

typedef enum
{
	_open,
	_not_found,
	_no_permission,
	_already_open
} t_file_status;

typedef enum
{
	_first_step,
	_sending_head,
	_sending_body,
	_body_sent
} t_send_status;

class Response
{
public:
	Response(void);
	Response(Response const &Response);
	~Response(void);

	void			   clear();
	void			   headAppend(std::string const &resp);
	void			   bodyAppend(std::string const &resp);
	void			   addHeader(std::string const &key, std::string const &value);
	std::string const &getHTTPVersion(void);
	std::string const &getHTTPStatus(void);
	std::string		   getIp(void);
	void			   setHTTPVersion(std::string const &http_version);
	void			   setHttpVersion(std::string const &http_version);
	;

	t_send_status		 send_status;
	std::string			 head;
	std::string			 body;
	size_t				 header_read_index;
	size_t				 header_write_index;
	size_t				 body_read_index;
	size_t				 body_write_index;
	size_t				 content_length;
	size_t				 sent_bytes;
	std::string			 http_version;
	std::string			 http_status;
	std::vector<uint8_t> send_buf;
	struct stat			 sb;
};

#endif
