#ifndef BACKENDHOLDER_HPP
#define BACKENDHOLDER_HPP

#include "CGIBackend.hpp"
#include "StaticBackend.hpp"

struct BackendHolder
{
	BackendHolder(t_backend_type backend_type, std::vector<std::string> const &cgi_args);
	BackendHolder(BackendHolder const &BackendHolder);
	~BackendHolder(void);

	t_backend_type			 backend_type;
	std::vector<std::string> cgi_args;
	ABackend				*addr;

private:
	BackendHolder(void) {}
};

#endif
