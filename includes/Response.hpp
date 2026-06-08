#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <async>
#include <container>
#include <string>
#include <sys/stat.h>
#include <vector>
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


// Only used for CGIBackend
typedef enum
{
	_init,
	_pending,
	_ready
} t_fetch_status;

class Response
{
public:
	Response(void);
	Response(Response const &Response);
	~Response(void);

	void			   clear();
	bool			   sendResponse(int fd);
	void			   sendResponse(container::Slice<uint8_t> &slice);
	bool			   ready(void);
	void			   headAppend(std::string const &resp);
	void			   bodyAppend(std::string const &resp);
	void			   addHeader(std::string const &key, std::string const &value);
	std::string const &getHTTPVersion(void);
	t_fetch_status	   getStatus(void);
	std::string const &getHTTPStatus(void);
	size_t			   getBodySize(void);
	std::string		   getIp(void);
	void			   setHTTPVersion(std::string const &http_version);
	void			   setHTTPStatus(std::string const &status, std::string const &message);
	void			   setHTTPStatus(std::string const &completeStatus);
	t_file_status	   openFile(std::string const &path);
	void			   setHttpVersion(std::string const &http_version);
	void			   setStatus(t_fetch_status status);
	void			   setGenerator(Request const &request, Response &response, async::Reactor &reactor, async::Io &bodyIo, int cgi_in_fd, int cgi_out_fd);
	void			   pollGenerator();
	bool			   read(size_t const &size);

private:
	void addBase(void);
	void sendHead(int fd);
	bool sendBody(int fd);
	void sendHead(container::Slice<uint8_t> &slice);
	void sendBody(container::Slice<uint8_t> &slice);

	t_send_status		 _send_status;
	t_fetch_status		 _fetch_status;
	int					 _file_fd;
	std::string			 _head;
	std::string			 _body;
	size_t				 _header_read_index;
	size_t				 _header_write_index;
	size_t				 _body_read_index;
	size_t				 _body_write_index;
	size_t				 _content_length;
	size_t				 _sent_bytes;
	std::string			 _http_version;
	std::string			 _http_status;
	std::vector<uint8_t> _send_buf;
	struct stat			 _sb;
	void				*_map;
	async::Future		*_response_generator;
};

#endif
