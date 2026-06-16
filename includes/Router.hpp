#ifndef ROUTER_H
#define ROUTER_H

#include <async>
#include <vector>
#include "ABackend.hpp"
#include "HTTPStream.hpp"
#include "RouterNode.hpp"
#include "url_utils.hpp"

class HTTPStream;

struct RouterRuleset
{
	RouterRuleset() : list_directories(_false), file_upload(_false), root(DEFAULT_ROOT) {}

	e_switch																	 list_directories;
	e_switch																	 file_upload;
	std::string																	 root;
	std::map<std::string, std::pair<std::string, e_redirect_type> >				 redirects;
	std::map<std::string, std::string>											 status_pages;
	std::string																	 dir_file;
	std::map<std::string, std::pair<t_backend_type, std::vector<std::string> > > backend_rules;
	std::vector<std::string>													 accepted_methods;
};

class Router
{
public:
	Router();
	~Router();

	Router			&operator=(Router const &Router);
	ABackend		*resolve(Request &request, Response &response, async::Reactor &reactor, HTTPStream &http_stream, bool force) const;
	void			 addServ(std::string const &domain, RouterNode const &node);
	static ABackend *getBackend(t_backend_type const &backend_type, std::vector<std::string> const &args, std::string const &root, async::Reactor &reactor,
								HTTPStream &http_stream);

private:
	static bool				 validateRequest(Request const &request, Response &response, HTTPStream &http_stream);
	RouterNode const		*getServerNode(std::string const &host, HTTPStream &http_stream) const;
	static RouterNode const *matchSubPath(std::string URI, std::map<std::string, RouterNode> const &sub_nodes);
	static RouterRuleset	 defineRuleset(std::string const &URI, RouterNode const &serv, RouterRuleset &ruleset);
	static ABackend *defineBackend(Request &request, Response &response, RouterRuleset const &ruleset, async::Reactor &reactor, HTTPStream &http_stream, bool force);

	std::map<std::string, RouterNode> _servers; // Servers identified by domain[:port(if other than 80)]
};

#endif
