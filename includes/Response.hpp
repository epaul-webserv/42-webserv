#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include <string>
# include <sys/stat.h>
# include "Request.hpp"

typedef enum
{
	_open,
	_not_found,
	_no_permission,
	_already_open
} t_file_status;

typedef enum
{
	_init,
	_head_sent,
	_body_sent
} t_resp_status;

class Response
{
public:
	Response(int fd, Request const &request);
	virtual ~Response(void);

	virtual bool sendResponse(void) = 0;
	virtual bool ready(void) = 0;

protected :
	virtual void setHeaders() = 0;
	void         headAppend(std::string const &resp);
	void         bodyAppend(std::string const &resp);

private :
	Response(void);
	Response(Response const &Response);

	t_resp_status  _status;
	int            _conn_fd;
	int            _file_fd;
	char           _head[8192];
	char *         _body;
	size_t         _header_read_index;
	size_t         _header_write_index;
	size_t         _body_read_index;
	size_t         _body_write_index;
	size_t         _content_length;
	size_t         _sent_bytes;
	struct stat    _sb;
	Request const &_request;
};

#endif
