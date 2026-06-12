#ifndef ABACKEND_HPP
#define ABACKEND_HPP

#include <async>

typedef enum
{
	_static,
	_cgi,
	_dir_index
} t_backend_type;

class ABackend : public async::Future
{
	// protected:
	// 	Request const							&_request;
	// 	std::string const						&_root;
	// 	Response								&_response;
	// 	async::Reactor							&_reactor;
	// 	async::BufIo<>							&_io;
	// 	std::map<std::string, std::string> const _status_pages;
	// 	ABackend(Request const &request, std::string const &root, Response &response, async::Reactor &reactor, async::BufIo<> &io,
	// 			 std::map<std::string, std::string> const status_pages);

public:
	virtual ~ABackend(void);
	virtual Poll poll() = 0;
};

#endif
