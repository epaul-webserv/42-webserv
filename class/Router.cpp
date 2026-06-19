#include "Router.hpp"
#include <html_generator.hpp>
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
							 HTTPStream &http_stream)
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
		case _dir_index:
			backend = new StaticBackend(root, http_stream);
			break;
		case _upload:
			backend = new UploadBackend(root, http_stream, http_stream.request_body());
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

bool Router::validateRequest(Request const &request, Response &response, HTTPStream &http_stream)
{
	const std::string http_version = request.getHTTPVersion();

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

RouterNode const *Router::matchSubPath(std::string URI, std::map<std::string, RouterNode> const &sub_nodes)
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

RouterRuleset Router::defineRuleset(std::string const &URI, RouterNode const &serv, RouterRuleset &ruleset)
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

ABackend *Router::defineBackend(Request &request, Response &response, RouterRuleset const &ruleset, async::Reactor &reactor, HTTPStream &http_stream, bool force)
{
	const std::map<std::string, std::pair<std::string, e_redirect_type> >::const_iterator matching_redirect = ruleset.redirects.find(request.getURI());
	if (!force && find(ruleset.accepted_methods, request.getMethod()) == ruleset.accepted_methods.end())
	{
		http_stream.setHTTPStatus("405", "Method Not Allowed");
		return (NULL);
	}
	if (!force && matching_redirect != ruleset.redirects.end())
	{
		switch (matching_redirect->second.second)
		{
		case _temporary:
			http_stream.setHTTPStatus("307", "Temporary Redirect");
			response.bodyAppend(get_html_error("307", "Temporary Redirect"));
			break;
		case _permanent:
			http_stream.setHTTPStatus("301", "Moved Permanently");
			response.bodyAppend(get_html_error("301", "Moved Permanently"));
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
	std::string																					 file_extension = get_file_ext(request.getURI());
	std::map<std::string, std::pair<t_backend_type, std::vector<std::string> > >::const_iterator it				= ruleset.backend_rules.find(file_extension);
	if (ruleset.backend_rules.find(file_extension) != ruleset.backend_rules.end())
		return (getBackend(it->second.first, it->second.second, ruleset.root, reactor, http_stream));
	if (request.getMethod() == "POST" && ruleset.file_upload == _true)
		return (getBackend(_upload, std::vector<std::string>(), ruleset.root, reactor, http_stream));
	return (getBackend(_static, std::vector<std::string>(), ruleset.root, reactor, http_stream));
}


/// @return ABackend pointer OR NULL if response has been set.
ABackend *Router::resolve(Request &request, Response &response, async::Reactor &reactor, HTTPStream &http_stream, bool force) const
{
	RouterRuleset ruleset;

	response.setHTTPVersion("HTTP/1.1");
	RouterNode const *serv = getServerNode(request.peekHeader("host"), http_stream);
	if (!serv)
		return (NULL);
	if (!force && !validateRequest(request, response, http_stream))
		return (NULL);
	ruleset					 = defineRuleset(request.getURI(), *serv, ruleset);
	http_stream.status_pages = ruleset.status_pages;
	return (defineBackend(request, response, ruleset, reactor, http_stream, force));
}

void Router::addServ(std::string const &domain, RouterNode const &node) { this->_servers.insert(std::pair<std::string, RouterNode>(domain, node)); }


#include <iostream>
#include <map>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include "Router.hpp"

// Helper function to print individual RouterNode structures recursively
static void printNode(const RouterNode &node, const std::string &indent)
{
	std::cout << indent << "Location: \"" << node.location << "\"\n";
	std::cout << indent << "  Root: \"" << node.root << "\"\n";

	std::cout << indent << "  Directory Listing: ";
	if (node.list_directories == _true)
		std::cout << "true\n";
	else if (node.list_directories == _false)
		std::cout << "false\n";
	else
		std::cout << "inherit\n";

	std::cout << indent << "  File Upload: ";
	if (node.file_upload == _true)
		std::cout << "true\n";
	else if (node.file_upload == _false)
		std::cout << "false\n";
	else
		std::cout << "inherit\n";

	// Print Accepted Methods
	if (!node.accepted_methods.empty())
	{
		std::cout << indent << "  Accepted Methods: [";
		for (size_t i = 0; i < node.accepted_methods.size(); ++i)
		{
			std::cout << "\"" << node.accepted_methods[i] << "\"";
			if (i + 1 < node.accepted_methods.size())
				std::cout << ", ";
		}
		std::cout << "]\n";
	}

	// Print index pages
	if (!node.index_pages.empty())
	{
		std::cout << indent << "  Index pages: [";
		for (size_t i = 0; i < node.index_pages.size(); ++i)
		{
			std::cout << "\"" << node.index_pages[i] << "\"";
			if (i + 1 < node.index_pages.size())
				std::cout << ", ";
		}
		std::cout << "]\n";
	}

	// Print Status Pages
	if (!node.status_pages.empty())
	{
		std::cout << indent << "  Status Pages:\n";
		for (std::map<std::string, std::string>::const_iterator it = node.status_pages.begin(); it != node.status_pages.end(); ++it)
		{
			std::cout << indent << "    " << it->first << " -> " << it->second << "\n";
		}
	}

	// Print Redirects
	if (!node.redirects.empty())
	{
		std::cout << indent << "  Redirects:\n";
		for (std::map<std::string, std::pair<std::string, e_redirect_type> >::const_iterator it = node.redirects.begin(); it != node.redirects.end(); ++it)
		{
			std::cout << indent << "    " << it->first << " -> " << it->second.first;
			std::cout << " (" << (it->second.second == _permanent ? "Permanent" : "Temporary") << ")\n";
		}
	}

	// Print Backend Rules
	if (!node.backend_rules.empty())
	{
		std::cout << indent << "  Backend Rules:\n";
		for (std::map<std::string, std::pair<t_backend_type, std::vector<std::string> > >::const_iterator it = node.backend_rules.begin();
			 it != node.backend_rules.end(); ++it)
		{
			std::cout << indent << "    Extension: ." << it->first << "\n";
			std::cout << indent << "      Type: " << (it->second.first == _cgi ? "CGI" : "Unknown") << "\n";
			std::cout << indent << "      Args: [";
			const std::vector<std::string> &args = it->second.second;
			for (size_t i = 0; i < args.size(); ++i)
			{
				std::cout << "\"" << args[i] << "\"";
				if (i + 1 < args.size())
					std::cout << ", ";
			}
			std::cout << "]\n";
		}
	}

	// Recursively print sub-nodes (nested locations)
	if (!node.sub_nodes.empty())
	{
		std::cout << indent << "  Sub-nodes (Locations):\n";
		for (std::map<std::string, RouterNode>::const_iterator it = node.sub_nodes.begin(); it != node.sub_nodes.end(); ++it)
		{
			std::cout << indent << "    Key: \"" << it->first << "\"\n";
			printNode(it->second, indent + "      ");
		}
	}
}

// Custom wrapper inside Router class to print its internal server configurations
void Router::printServers() const
{
	if (_servers.empty())
	{
		std::cout << "    (No virtual server_name keys registered inside this Router instance)\n";
		return;
	}

	for (std::map<std::string, RouterNode>::const_iterator it = _servers.begin(); it != _servers.end(); ++it)
	{
		std::cout << "  Server Name (Key): \"" << it->first << "\"\n";
		printNode(it->second, "    ");
		std::cout << "  -------------------------------------\n";
	}
}
