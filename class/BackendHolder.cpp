#include "BackendHolder.hpp"

BackendHolder::BackendHolder(t_backend_type		backend_type,
							 std::string const &cgi_path) :
	backend_type(backend_type), cgi_path(cgi_path)
{
	// if (backend_type == _cgi)
	// 	this->addr = (new CGIBackend(cgi_path));
	// else
	(void)backend_type;
	(void)cgi_path;
	// This function is waiting for CGIBackend to be implemented
	this->addr = (new StaticBackend());
}

BackendHolder::BackendHolder(BackendHolder const &BackendHolder) :
	backend_type(BackendHolder.backend_type), cgi_path(BackendHolder.cgi_path)
{
	// if (backend_type == _cgi)
	// 	this->addr = (new CGIBackend(cgi_path));
	// else
	(void)backend_type;
	(void)cgi_path;
	// This function is waiting for CGIBackend to be implemented
	this->addr = (new StaticBackend());
}

BackendHolder::~BackendHolder(void) { delete this->addr; }
