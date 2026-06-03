//
// Created by logan on 5/28/26.
//

#include "Router.hpp"
#include <iostream>
#include "StaticBackend.hpp"

Router::Router() { this->_backends.push_back(BackendHolder(_static, std::vector<std::string>(0))); }

Router::~Router()
{
}

Router &Router::operator=(Router const &Router)
{
	this->_backends = Router._backends;
	this->_rules    = Router._rules;
	return (*this);
}

void Router::addBackend(t_backend_type const &type, std::vector<std::string> const &args)
{
	if (type == _cgi)
		this->_backends.push_back(BackendHolder(type, args));
	else
		std::cerr << "Backend type not implemented yet" << std::endl;
	(void)type;
}

void Router::addRule(t_backend_type const &type, std::vector<std::string> const &args, std::string const &domain, std::string file_extension)
{
	if (!getBackend(type, args))
		addBackend(type, args);
	std::string tmp = to_lower(domain);
	this->_rules.push_back(backend_rule(domain, file_extension, getBackend(type, args)));
}

ABackend *Router::getBackend(t_backend_type const &backend_type, std::vector<std::string> const &args) const
{
	std::list<BackendHolder>::const_iterator it;
	std::list<BackendHolder>::const_iterator ite = this->_backends.end();

	for (it = this->_backends.begin(); it != ite; ++it)
	{
		if (backend_type == _static && it->backend_type == backend_type)
			return (it->addr);
		if (it->backend_type == backend_type && it->cgi_args == args)
			return (it->addr);
	}
	return (NULL);
}

void Router::resolve(Request const &request, Response &response) const
{
	std::string                             domain         = to_lower(request.peekHeader("host"));
	std::string                             file_extension = get_file_ext(request.getURI());
	std::list<backend_rule>::const_iterator it;
	std::list<backend_rule>::const_iterator ite     = this->_rules.end();
	ABackend *                              backend = NULL;

	for (it = this->_rules.begin(); it != ite; ++it)
	{
		if (it->domain == domain && it->file_extension == file_extension)
		{
			backend = it->addr;
			break;
		}
	}
	if (!backend)
		backend = (this->_backends.front().addr);
	backend->generateResponse(request, response);
}
