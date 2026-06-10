#ifndef ABACKEND_HPP
#define ABACKEND_HPP

#include <async>
#include "Request.hpp"
#include "Response.hpp"

typedef enum
{
	_static,
	_cgi,
	_fast_cgi
} t_backend_type;

class ABackend
{
public:
	ABackend(void);
	virtual ~ABackend(void);

	virtual void generateResponse(Request const &request, std::string const &root, Response &response, async::Reactor &reactor, async::BufIo<> &io) = 0;
};

#endif
