#ifndef BACKENDHOLDER_HPP
#define BACKENDHOLDER_HPP

// #include "CGIBackend.hpp"
#include "StaticBackend.hpp"

struct BackendHolder
{
	BackendHolder(t_backend_type backend_type, std::string const &cgi_path);
	BackendHolder(BackendHolder const &BackendHolder);
	~BackendHolder(void);

	t_backend_type backend_type;
	std::string	   cgi_path;
	ABackend	  *addr;

private:
	BackendHolder(void) {}
};

#endif
