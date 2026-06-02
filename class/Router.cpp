//
// Created by logan on 5/28/26.
//

#include "Router.hpp"
#include <iostream>
#include "StaticBackend.hpp"

Router::Router() { this->_backends.push_back(BackendHolder(_static, "")); }

Router::~Router() {}

void Router::addBackend(t_backend_type const &type, std::string const &path)
{
	if (type == _cgi)
		this->_backends.push_back(BackendHolder(type, path));
	else
		std::cerr << "Backend type not implemented yet" << std::endl;
	(void)type;
	(void)path;
}

void Router::addRule(t_backend_type const &type, std::string const &path,
					 std::string const &domain, std::string file_extension)
{
	if (!getBackend(type, path))
		addBackend(type, path);
	std::string tmp = to_lower(domain);
	this->_rules.push_back(
		backend_rule(domain, file_extension, getBackend(type, path)));
}

ABackend *Router::getBackend(t_backend_type const &backend_type,
							 std::string const	  &cgi_path) const
{
	std::vector<BackendHolder>::const_iterator it;
	std::vector<BackendHolder>::const_iterator ite = this->_backends.end();

	for (it = this->_backends.begin(); it != ite; ++it)
	{
		if (backend_type == _static && it->backend_type == backend_type)
			return (it->addr);
		if (it->backend_type == backend_type && it->cgi_path == cgi_path)
			return (it->addr);
	}
	return (NULL);
}

void Router::resolve(Request const &request, Response &response) const
{
	std::string domain		   = to_lower(request.peekHeader("host"));
	std::string file_extension = get_file_ext(request.getURI());
	std::vector<backend_rule>::const_iterator it;
	std::vector<backend_rule>::const_iterator ite	  = this->_rules.end();
	ABackend								 *backend = NULL;

	for (it = this->_rules.begin(); it != ite; ++it)
	{
		if (it->domain == domain && it->file_extension == file_extension)
		{
			backend = it->addr;
			break;
		}
	}
	if (!backend)
		backend = (this->_backends[0].addr);
	backend->generateResponse(request, response);
}
