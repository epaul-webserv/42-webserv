//
// Created by logan on 5/28/26.
//

#include "Router.hpp"
#include <iostream>
#include "CGIBackend.hpp"

static t_file_type fileType(std::string const &path, std::string const &root)
{
	struct stat s = {};

	if (stat((root + path).c_str(), &s) == -1)
	{
		if (errno == EACCES)
			return (_no_access);
		if (errno == ENOMEM)
			return (_err);
		return (_none);
	}
	switch (s.st_mode & S_IFMT)
	{
	case S_IFDIR:
		return (_dir);
	case S_IFREG:
		return (_file);
	default:
		return (_none);
	}
}

Router::Router() {}

Router::~Router() {}

Router &Router::operator=(Router const &router)
{
	this->_servers = router._servers;
	return (*this);
}

ABackend *Router::getBackend(t_backend_type const &backend_type, std::vector<std::string> const &args, Request const &request, std::string const &root,
							 Response &response, async::Reactor &reactor, async::BufIo<> &io, std::map<std::string, std::string> const &status_pages) const
{
	ABackend *backend = NULL;
	switch (backend_type)
	{
	case _static:
		backend = new StaticBackend(request, root, response, reactor, io, status_pages);
	case _cgi:
		backend = new CGIBackend(args, request, root, response, reactor, io, status_pages);
	case _dir_index:
		backend = new DirectoryIndexBackend(request, root, response, reactor, io, status_pages);
	}
	if (!backend)
	{
		std::cerr << "[ERROR] Backend creation has failed : " << std::strerror(errno) << std::endl;
		response.setHTTPStatus("500", "Internal Server Error", status_pages);
	}
	return (backend);
}

bool Router::validateRequest(Request const &request, Response &response, std::map<std::string, std::string> const &status_pages) const
{
	std::string http_version = request.getHTTPVersion();

	if (http_version == "HTTP/1.0" || http_version == "HTTP/1.1")
	{
		if (http_version == "HTTP/1.1" && request.peekHeader("connection") != "close")
			response.addHeader("Connection", "keep-alive");
		response.setHTTPVersion(http_version);
	}
	else if (!http_version.empty()) // Empty is HTTP/0.9
	{
		response.setHTTPVersion("HTTP/1.1");
		response.setHTTPStatus("505", "HTTP Version Not Supported", status_pages);
		response.setStatus(_ready);
		return (false);
	}
	if (request.getURI().empty())
	{
		response.setHTTPStatus("400", "Bad Request", status_pages);
		response.setStatus(_ready);
		return (false);
	}
	return (true);
}

RouterNode const *Router::getServerNode(std::string const &host, Response &response) const
{
	RouterNode										 *server_node = NULL;
	std::map<std::string, RouterNode>::const_iterator it		  = this->_servers.find(host);

	if (it == this->_servers.end())
	{
		response.setHTTPStatus("404", "Not Found", std::map<std::string, std::string>());
		return (NULL);
	}
	return (&it->second);
}

RouterNode const *Router::matchSubPath(std::string URI, std::map<std::string, RouterNode> const &sub_nodes) const
{
	std::map<std::string, RouterNode>::const_iterator it;
	while (!URI.empty())
	{
		URI = URI.substr(0, URI.find_last_of('/') - 1);
		it	= sub_nodes.find(URI);
		if (it != sub_nodes.end())
			return (&it->second);
	}
	return (NULL);
}

RouterRuleset Router::defineRuleset(std::string const &URI, RouterNode const &serv, RouterRuleset &ruleset) const
{
	RouterNode const *matching_sub_node = NULL;

	ruleset.list_directories = serv.list_directories;
	if (!serv.root.empty())
		ruleset.root = serv.root;
	ruleset.redirects.insert(serv.redirects.begin(), serv.redirects.end());
	ruleset.status_pages.insert(serv.status_pages.begin(), serv.status_pages.end());
	if (!serv.dir_file.empty())
		ruleset.dir_file = serv.dir_file;
	ruleset.backend_rules.insert(serv.backend_rules.begin(), serv.backend_rules.end());
	if (ruleset.redirects.find(URI) != ruleset.redirects.end())
		return (ruleset);
	matching_sub_node = matchSubPath(URI, serv.sub_nodes);
	if (matching_sub_node)
		return (defineRuleset(URI, *matching_sub_node, ruleset));
	if (!serv.accepted_methods.empty())
		return (ruleset);
}

static std::vector<std::string>::const_iterator find(std::vector<std::string> const &vec, std::string const &to_find)
{
	std::vector<std::string>::const_iterator it;
	std::vector<std::string>::const_iterator ite = vec.end();

	for (it = vec.begin(); it != ite; ++it)
	{
		if (to_find == *it)
			return (it);
	}
	return (it);
}

ABackend *Router::defineBackend(Request &request, Response &response, RouterRuleset const &ruleset, async::Reactor &reactor, async::BufIo<> &io) const
{
	std::map<std::string, std::pair<std::string, e_redirect_type> >::const_iterator matching_redirect = ruleset.redirects.find(request.getURI());
	if (find(ruleset.accepted_methods, request.getMethod()) == ruleset.accepted_methods.end())
	{
		response.setHTTPStatus("405", "Method Not Allowed", ruleset.status_pages);
		return (NULL);
	}
	if (matching_redirect != ruleset.redirects.end())
	{
		switch (matching_redirect->second.second)
		{
		case _temporary:
			response.setHTTPStatus("307", "Temporary Redirect", ruleset.status_pages);
			break;
		case _permanent:
			response.setHTTPStatus("301", "Moved Permanently", ruleset.status_pages);
		}
		response.addHeader("Location", matching_redirect->second.first);
		response.setStatus(_ready);
		return (NULL);
	}
	if (fileType(request.getURI(), ruleset.root) == _dir)
	{
		if (ruleset.list_directories)
		{
			// Check the allocation in getBackend
			return (getBackend(_dir_index, std::vector<std::string>(), request, ruleset.root, response, reactor, io, ruleset.status_pages));
		}
		if (!ruleset.dir_file.empty())
		{
			request.setURI(ruleset.dir_file);
			return (getBackend(_static, std::vector<std::string>(), request, "", response, reactor, io, ruleset.status_pages));
		}
	}
	else
	{
		std::string																					 file_extension = get_file_ext(request.getURI());
		std::map<std::string, std::pair<t_backend_type, std::vector<std::string> > >::const_iterator it				= ruleset.backend_rules.find(file_extension);
		if (ruleset.backend_rules.find(file_extension) != ruleset.backend_rules.end())
			return (getBackend(it->second.first, it->second.second, request, ruleset.root, response, reactor, io, ruleset.status_pages));
		else
			return (getBackend(_static, std::vector<std::string>(), request, ruleset.root, response, reactor, io, ruleset.status_pages));
	}
	return (NULL);
}

/// @return ABackend pointer OR NULL if response has been set.
ABackend *Router::resolve(Request &request, Response &response, async::Reactor &reactor, async::BufIo<> &io) const
{
	RouterNode const *serv;
	RouterRuleset	  ruleset;

	response.setHTTPVersion("HTTP/1.1");
	serv = getServerNode(request.peekHeader("host"), response);
	if (!serv)
		return (NULL);
	if (!validateRequest(request, response, serv->status_pages))
		return (NULL);
	ruleset = defineRuleset(request.getURI(), *serv, ruleset);
	return (defineBackend(request, response, ruleset, reactor, io));
}

void Router::addServ(std::string const &domain, RouterNode const &node) { this->_servers.insert(std::pair<std::string, RouterNode>(domain, node)); }
