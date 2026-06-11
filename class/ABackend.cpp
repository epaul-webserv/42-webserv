#include "ABackend.hpp"
#include "html_generator.hpp"

ABackend::ABackend(Request const &request, std::string const &root, Response &response, async::Reactor &reactor, async::BufIo<> &io,
				   std::map<std::string, std::string> const status_pages) :
	_request(request), _root(root), _response(response), _reactor(reactor), _io(io), _status_pages(status_pages)
{
}

ABackend::~ABackend(void) {}
