#include "CGIResponseGenerator.hpp"

CGIResponseGenerator::CGIResponseGenerator(Request const &request, Response &response, async::Reactor &reactor, async::Io &bodyIo, int cgi_in_fd, int cgi_out_fd) :
	_request(request), _response(response), _bodyIo(bodyIo), _cgi_in(new async::Io(reactor, cgi_in_fd)), _cgi_out(new async::Io(reactor, cgi_out_fd))
{
}

async::Future::Poll CGIResponseGenerator::poll() throw(async::OsError) {
	return Ready; }

CGIResponseGenerator::~CGIResponseGenerator()
{
	delete _cgi_in;
	delete _cgi_out;
}
