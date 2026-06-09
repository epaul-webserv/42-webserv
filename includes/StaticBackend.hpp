#ifndef STATIC_BACKEND_HPP
#define STATIC_BACKEND_HPP

#include <async>
#include <string>
#include <sys/stat.h>
#include "ABackend.hpp"

struct s_staticbackend_conf
{
	std::map<std::string, std::string> mime_type;
};

class StaticBackend : public ABackend
{
public:
	StaticBackend(void);
	StaticBackend(s_staticbackend_conf const &conf);
	~StaticBackend(void);

	t_file_status openFile(std::string path);
	void		  generateResponse(Request const &request, Response &response, async::Reactor &Reactor, async::BufIo<> &io);

private:
	void		  setHeaders();
	void		  sendHead(void);
	bool		  sendBody(void);
	void		  setContentType(std::string const &file_ext, Response &response);
	void		  response_0_9(Request const &request, Response &response, async::Reactor &reactor);
	void		  response_1_0(Request const &request, Response &response, async::Reactor &reactor);
	void		  response_1_1(Request const &request, Response &response, async::Reactor &reactor);
	t_file_status openFile(std::string const &path, Response &response);

	void							  *_map;
	struct stat						   _sb;
	std::map<std::string, std::string> _mime_type;
};

#endif
