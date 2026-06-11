#ifndef ROUTER_H
#define ROUTER_H

#include <async>
#include <list>
#include <vector>
#include "ABackend.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "RouterNode.hpp"
#include "url_utils.hpp"

struct RouterRuleset
{
	RouterRuleset() : list_directories(true), root(DEFAULT_ROOT) {}
	bool																		 list_directories;
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

	Router	 &operator=(Router const &Router);
	ABackend *resolve(Request &request, Response &response, async::Reactor &reactor, async::BufIo<> &io) const;

private:
	ABackend *getBackend(t_backend_type const &backend_type, std::vector<std::string> const &args, Request const &request, std::string const &root, Response &response,
						 async::Reactor &reactor, async::BufIo<> &io, std::map<std::string, std::string> const &status_pages) const;
	bool	  validateRequest(Request const &request, Response &response, std::map<std::string, std::string> const &status_pages) const;
	RouterNode const *getServerNode(std::string const &host, Response &response) const;
	RouterNode const *matchSubPath(std::string URI, std::map<std::string, RouterNode> const &sub_nodes) const;
	RouterRuleset	  defineRuleset(std::string const &URI, RouterNode const &serv, RouterRuleset &ruleset) const;
	ABackend		 *defineBackend(Request &request, Response &response, RouterRuleset const &ruleset, async::Reactor &reactor, async::BufIo<> &io) const;

	std::map<std::string, RouterNode> _servers; // Servers identified by domain[:port(if other than 80)]
};

#endif
