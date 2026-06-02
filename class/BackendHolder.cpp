#include "BackendHolder.hpp"

BackendHolder::BackendHolder(t_backend_type		backend_type,
							 std::string const &cgi_path) :
	backend_type(backend_type), cgi_path(cgi_path)
{
	s_staticbackend_conf test;

	test.mime_type.insert(
		std::pair<std::string, std::string>("svg", "image/svg+xml"));
	test.mime_type.insert(
		std::pair<std::string, std::string>("png", "image/png"));
	if (backend_type == _cgi)
		this->addr = (new CGIBackend(cgi_path));
	else
		this->addr = (new StaticBackend(test));
}

BackendHolder::BackendHolder(BackendHolder const &BackendHolder) :
	backend_type(BackendHolder.backend_type), cgi_path(BackendHolder.cgi_path)
{
	s_staticbackend_conf test;

	test.mime_type.insert(
		std::pair<std::string, std::string>("svg", "image/svg+xml"));
	test.mime_type.insert(
		std::pair<std::string, std::string>("png", "image/png"));
	if (backend_type == _cgi)
		this->addr = (new CGIBackend(cgi_path));
	else
		this->addr = (new StaticBackend(test));
}

BackendHolder::~BackendHolder(void) { delete this->addr; }
