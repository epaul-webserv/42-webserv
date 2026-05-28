#ifndef STATIC_BACKEND_HPP
#define STATIC_BACKEND_HPP

#include <string>
#include <sys/stat.h>
#include "ABackend.hpp"

class StaticBackend : public ABackend
{
public:
	StaticBackend(void);
	~StaticBackend(void);

	t_file_status openFile(std::string path);
	void		  generateResponse(Request const &request, Response &response);

private:
	void setHeaders();
	void sendHead(void);
	bool sendBody(void);
	void response_0_9(Request const &request, Response &response);
	void response_1_0(Request const &request, Response &response);
	void response_1_1(Request const &request, Response &response);

	void	   *_map;
	struct stat _sb;
};

#endif
