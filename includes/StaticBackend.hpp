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

	void generateResponse(Request const &request, Response &response, async::Reactor &Reactor, async::BufIo<> &io);

private:
	void		  setContentType(std::string const &file_ext, Response &response);
	t_file_status openFile(std::string const &path, Response &response);

	void							  *_map;
	struct stat						   _sb;
	std::map<std::string, std::string> _mime_type;
};

#endif
