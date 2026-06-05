#include "webserv.h"

class CGIResponseGenerator : public async::Future
{
public:
	CGIResponseGenerator(Request const &request, Response &response, async::Reactor &reactor, async::Io &bodyIo, int cgi_in_fd, int cgi_out_fd);
	Future::Poll poll() throw(async::OsError);
	~CGIResponseGenerator();

private:
	Request const &_request;
	Response	  &_response;
	async::Io	  &_bodyIo;
	async::Io	  *_cgi_in;
	async::Io	  *_cgi_out;
};
