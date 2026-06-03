#ifndef ROUTER_H
#define ROUTER_H

#include <map>
#include <set>
#include <vector>
#include "ABackend.hpp"
#include "BackendHolder.hpp"
// #include "CGIBackend.hpp"
#include <list>
#include "Request.hpp"
#include "Response.hpp"
#include "StaticBackend.hpp"
#include "url_utils.hpp"

struct backend_rule
{
	backend_rule(std::string domain, std::string file_extension, ABackend *addr) :
		domain(domain), file_extension(file_extension), addr(addr)
	{
	}

	std::string domain;
	std::string file_extension;
	ABackend *  addr;
};

class Router
{
public:
	Router();
	~Router();

	Router &operator=(Router const &Router);
	void    addBackend(t_backend_type const &type, std::vector<std::string> const &args);
	void    addRule(t_backend_type const &type, std::vector<std::string> const &args, std::string const &domain, std::string file_extension);
	void    resolve(Request const &request, Response &response) const;

private:
	ABackend *getBackend(t_backend_type const &type, std::vector<std::string> const &args) const;

	std::list<BackendHolder> _backends;
	std::list<backend_rule>  _rules;
};

#endif
