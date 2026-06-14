#include "Router.hpp"
#include <iostream>
#include "CGIBackend.hpp"
#include "StaticBackend.hpp"
#include "UploadBackend.hpp"

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

ABackend *Router::getBackend(t_backend_type const &backend_type, std::vector<std::string> const &args, std::string const &root, async::Reactor &reactor,
							 HTTPStream &http_stream) const
{
	ABackend *backend = NULL;
	try
	{
		switch (backend_type)
		{
		case _static:
			backend = new StaticBackend(root, http_stream);
			break;
		case _cgi:
			backend = new CGIBackend(args, root, reactor, http_stream);
			break;
			// case _dir_index:
			// 	backend = new DirectoryIndexBackend(request, root, response, reactor, io, status_pages);
		case _upload:
			backend = new UploadBackend(root, http_stream, http_stream._request_body);
			break;
		default:
			backend = new CGIBackend(args, root, reactor, http_stream);
		}
	}
	catch (std::bad_alloc &e)
	{
		std::cerr << "[ERROR] Backend creation has failed : " << e.what() << std::endl;
		http_stream.setHTTPStatus("500", "Internal Server Error");
	}
	return (backend);
}

bool Router::validateRequest(Request const &request, Response &response, HTTPStream &http_stream) const
{
	std::string http_version = request.getHTTPVersion();

	if (http_version == "HTTP/1.0" || http_version == "HTTP/1.1")
	{
		response.setHTTPVersion(http_version);
		if (http_version == "HTTP/1.1")
		{
			if (request.peekHeader("host").empty())
			{
				http_stream.setHTTPStatus("400", "Bad Request");
				response.addHeader("Connection", "close");
				return (false);
			}
			if (request.peekHeader("connection") != "close")
				response.addHeader("Connection", "keep-alive");
		}
	}
	else if (!http_version.empty()) // Empty is HTTP/0.9
	{
		response.setHTTPVersion("HTTP/1.1");
		http_stream.setHTTPStatus("505", "HTTP Version Not Supported");
		return (false);
	}
	if (request.getURI().empty())
	{
		http_stream.setHTTPStatus("400", "Bad Request");
		return (false);
	}
	return (true);
}

RouterNode const *Router::getServerNode(std::string const &host, HTTPStream &http_stream) const
{
	std::map<std::string, RouterNode>::const_iterator it;

	if (host.empty())
		it = this->_servers.begin();
	else
		it = this->_servers.find(host);
	if (it == this->_servers.end())
	{
		http_stream.setHTTPStatus("404", "Not Found");
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

	if (serv.list_directories)
		ruleset.list_directories = serv.list_directories;
	if (serv.file_upload)
		ruleset.file_upload = serv.file_upload;
	if (!serv.accepted_methods.empty())
		ruleset.accepted_methods = serv.accepted_methods;
	if (!serv.root.empty())
		ruleset.root = serv.root;
	ruleset.redirects.insert(serv.redirects.begin(), serv.redirects.end());
	ruleset.status_pages.insert(serv.status_pages.begin(), serv.status_pages.end());
	if (!serv.dir_file.empty())
		ruleset.dir_file = serv.dir_file;
	ruleset.backend_rules.insert(serv.backend_rules.begin(), serv.backend_rules.end());
	if (ruleset.redirects.find(URI) != ruleset.redirects.end())
		return (ruleset);
	if (!serv.sub_nodes.empty())
		matching_sub_node = matchSubPath(URI, serv.sub_nodes);
	if (matching_sub_node)
		return (defineRuleset(URI, *matching_sub_node, ruleset));
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

ABackend *Router::defineBackend(Request &request, Response &response, RouterRuleset const &ruleset, async::Reactor &reactor, HTTPStream &http_stream) const
{
	std::map<std::string, std::pair<std::string, e_redirect_type> >::const_iterator matching_redirect = ruleset.redirects.find(request.getURI());
	if (find(ruleset.accepted_methods, request.getMethod()) == ruleset.accepted_methods.end())
	{
		http_stream.setHTTPStatus("405", "Method Not Allowed");
		return (NULL);
	}
	if (matching_redirect != ruleset.redirects.end())
	{
		switch (matching_redirect->second.second)
		{
		case _temporary:
			http_stream.setHTTPStatus("307", "Temporary Redirect");
			break;
		case _permanent:
			http_stream.setHTTPStatus("301", "Moved Permanently");
		}
		response.addHeader("Location", matching_redirect->second.first);
		return (NULL);
	}
	if (fileType(request.getURI(), ruleset.root) == _dir)
	{
		if (ruleset.list_directories == _true)
			return (getBackend(_dir_index, std::vector<std::string>(), ruleset.root, reactor, http_stream));
		if (!ruleset.dir_file.empty())
		{
			request.setURI(ruleset.dir_file);
			return (getBackend(_static, std::vector<std::string>(), "", reactor, http_stream));
		}
		http_stream.setHTTPStatus("403", "Forbidden");
		// FIXME: ADD DEFAULT INDEX PAGES
		return (NULL);
	}
	else
	{
		std::string																					 file_extension = get_file_ext(request.getURI());
		std::map<std::string, std::pair<t_backend_type, std::vector<std::string> > >::const_iterator it				= ruleset.backend_rules.find(file_extension);
		if (ruleset.backend_rules.find(file_extension) != ruleset.backend_rules.end())
			return (getBackend(it->second.first, it->second.second, ruleset.root, reactor, http_stream));
		if (request.getMethod() == "POST" && ruleset.file_upload == _true)
			return (getBackend(_upload, std::vector<std::string>(), ruleset.root, reactor, http_stream));
		return (getBackend(_static, std::vector<std::string>(), ruleset.root, reactor, http_stream));
	}
	return (NULL);
}

/// @return ABackend pointer OR NULL if response has been set.
ABackend *Router::resolve(Request &request, Response &response, async::Reactor &reactor, HTTPStream &http_stream) const
{
	RouterRuleset ruleset;

	response.setHTTPVersion("HTTP/1.1");
	RouterNode const *serv = getServerNode(request.peekHeader("host"), http_stream);
	if (!serv)
		return (NULL);
	if (!validateRequest(request, response, http_stream))
		return (NULL);
	ruleset = defineRuleset(request.getURI(), *serv, ruleset);
	return (defineBackend(request, response, ruleset, reactor, http_stream));
}

void Router::addServ(std::string const &domain, RouterNode const &node) { this->_servers.insert(std::pair<std::string, RouterNode>(domain, node)); }
