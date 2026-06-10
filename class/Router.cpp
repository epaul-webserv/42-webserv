//
// Created by logan on 5/28/26.
//

#include "Router.hpp"
#include <iostream>
#include "StaticBackend.hpp"

enum e_redirect_type
{
	_permanent,
	_temporary
};

struct RouterNode
{
	RouterNode(void) : list_directories(false), request_path("/") {}
	RouterNode(std::string const &path) : list_directories(false), request_path(path) {}

	bool														   list_directories;
	std::string const											   request_path;
	std::string													   root; // path to the root of the files to serve, inactive if empty
	std::map<std::string, RouterNode>							   sub_nodes; // relative (to active node URI) URI of a sub path, leading to another rule subset
	std::map<std::string, std::pair<std::string, e_redirect_type>> redirects; // URI to redirect, new location and type of redirection
	std::map<std::string, std::string>							   status_pages; // status, relative (to applicable root) path of page to serve in case of this status;
	std::string dir_file; // relative (to applicable root) path to the file to serve in case on a dir ressource, inactive if empty
};

Router::Router() { this->_backends.push_back(BackendHolder(_static, std::vector<std::string>(0))); }

Router::~Router() {}

Router &Router::operator=(Router const &Router)
{
	this->_backends = Router._backends;
	this->_rules	= Router._rules;
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

void Router::addRule(t_backend_type const &type, std::vector<std::string> const &args, IBackendRule &rule)
{
	if (!getBackend(type, args))
		addBackend(type, args);
	rule.addr = getBackend(type, args);
	this->_rules.push_back(rule);
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

/// return ABackend pointer OR NULL if response has been set.
ABackend *Router::resolve(Request const &request, Response &response) const {}

// void Router::resolve(Request const &request, Response &response) const
// {
// 	std::string								domain		   = to_lower(request.peekHeader("host"));
// 	std::string								file_extension = get_file_ext(request.getURI());
// 	std::list<backend_rule>::const_iterator it;
// 	std::list<backend_rule>::const_iterator ite		= this->_rules.end();
// 	ABackend							   *backend = NULL;
//
// 	if (request.getHTTPVersion().empty())
// 	{
// 	}
// 	else if (request.getHTTPVersion() == "HTTP/1.0" || request.getHTTPVersion() == "HTTP/1.1")
// 	{
// 		if (request.getHTTPVersion() == "HTTP/1.1" && request.peekHeader("connection") != "close")
// 			response.addHeader("Connection", "keep-alive");
// 		response.setHTTPVersion(request.getHTTPVersion());
// 	}
// 	else
// 	{
// 		response.setHTTPVersion("HTTP/1.1");
// 		response.setError("505", "HTTP Version Not Supported");
// 		response.setStatus(_ready);
// 		return;
// 	}
// 	if (request.getURI().empty())
// 	{
// 		response.setHTTPVersion("HTTP/1.1");
// 		response.setError("400", "Bad Request");
// 	}
// 	for (it = this->_rules.begin(); it != ite; ++it)
// 	{
// 		if (it->domain == domain && it->file_extension == file_extension)
// 		{
// 			backend = it->addr;
// 			break;
// 		}
// 	}
// 	if (!backend)
// 		backend = (this->_backends.front().addr);
// 	backend->generateResponse(request, response, reactor, client_io);
// }
