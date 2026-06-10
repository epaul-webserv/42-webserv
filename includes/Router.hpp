#ifndef ROUTER_H
#define ROUTER_H

#include <async>
#include <list>
#include <vector>
#include "ABackend.hpp"
#include "BackendHolder.hpp"
#include "IBackendRule.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "url_utils.hpp"

class Router
{
public:
	Router();
	~Router();

	Router	 &operator=(Router const &Router);
	void	  addBackend(t_backend_type const &type, std::vector<std::string> const &args);
	void	  addRule(t_backend_type const &type, std::vector<std::string> const &args, IBackendRule &rule);
	ABackend *resolve(Request const &request, Response &response) const;

private:
	ABackend *getBackend(t_backend_type const &type, std::vector<std::string> const &args) const;

	std::list<BackendHolder>														  _backends;
	std::list<IBackendRule>															  _rules;
	std::map<std::string, std::map<std::string, std::map<std::string, IBackendRule>>> _eigth_wonder_of_the_world;
};

#endif
