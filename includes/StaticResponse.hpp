#ifndef STATIC_RESPONSE_HPP
# define STATIC_RESPONSE_HPP

# include <string>
# include <sys/stat.h>
# include "Response.hpp"

class StaticResponse : public Response
{
public:
	StaticResponse(int fd, Request const &request) :
		Response(fd, request)
	{
	}

	// void                clear(int fd);
	// void                prepareNext(void);
	t_file_status openFile(std::string path);
	bool          sendResponse(void) override;

private:
	void setHeaders();
	void sendHead(void);
	bool sendBody(void);

	void *      _map;
	struct stat _sb;
};

#endif
