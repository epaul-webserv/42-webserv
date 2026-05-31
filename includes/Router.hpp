#ifndef ROUTER_H
#define ROUTER_H

#include <set>
#include <map>
#include <vector>
#include "ABackend.hpp"
#include "BackendHolder.hpp"
#include "CGIBackend.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "StaticBackend.hpp"
#include "url_utils.hpp"

struct backend_rule
{
	backend_rule(std::string domain, std::string file_extension,
				ABackend *   addr) :
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

	void addBackend(t_backend_type const &type, std::string const &path = "");
	void addRule(t_backend_type const &type, std::string const &path,
				std::string const &    domain, std::string      file_extension);
	void resolve(Request const &request, Response &response) const;

private:
	ABackend *getBackend(t_backend_type const &type,
						std::string const &    path) const;

	std::vector<BackendHolder> _backends;
	std::vector<backend_rule>  _rules;
};

#endif
