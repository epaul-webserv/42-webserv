#ifndef STATIC_BACKEND_HPP
#define STATIC_BACKEND_HPP

#include <async>
#include <string>
#include <sys/stat.h>
#include "ABackend.hpp"


class StaticBackend : public ABackend
{
public:
	StaticBackend(Request const &request, std::string const &root, Response &response, async::Reactor &reactor, async::BufIo<> &io,
				  std::map<std::string, std::string> const &status_pages);
	~StaticBackend(void);

	void generateResponse(Request const &request, Response &response, async::Reactor &Reactor, async::BufIo<> &io);

private:
	void		  setContentType(std::string const &file_ext, Response &response);
	t_file_status openFile(std::string const &path, Response &response);

	struct stat _sb;
};

#endif
