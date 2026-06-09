#include "BackendHolder.hpp"

BackendHolder::BackendHolder(t_backend_type backend_type, std::vector<std::string> const &cgi_args) : backend_type(backend_type), cgi_args(cgi_args)
{
	s_staticbackend_conf test;

	test.mime_type.insert(std::pair<std::string, std::string>("svg", "image/svg+xml"));
	test.mime_type.insert(std::pair<std::string, std::string>("png", "image/png"));
	test.mime_type.insert(std::pair<std::string, std::string>("html", "text/html"));
	if (backend_type == _cgi)
		this->addr = (new CGIBackend(cgi_args));
	else
		this->addr = (new StaticBackend(test));
}

BackendHolder::BackendHolder(BackendHolder const &BackendHolder) : backend_type(BackendHolder.backend_type), cgi_args(BackendHolder.cgi_args)
{
	s_staticbackend_conf test;

	test.mime_type.insert(std::pair<std::string, std::string>("svg", "image/svg+xml"));
	test.mime_type.insert(std::pair<std::string, std::string>("png", "image/png"));
	test.mime_type.insert(std::pair<std::string, std::string>("html", "text/html"));
	if (backend_type == _cgi)
		this->addr = (new CGIBackend(cgi_args));
	else
		this->addr = (new StaticBackend(test));
}

BackendHolder::~BackendHolder(void) { delete this->addr; }
