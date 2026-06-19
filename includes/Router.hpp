#ifndef ROUTER_H
#define ROUTER_H

#include <async>
#include <vector>
#include "ABackend.hpp"
#include "HTTPStream.hpp"
#include "RouterNode.hpp"
#include "RouterRuleset.hpp"
#include "url_utils.hpp"

class HTTPStream;

class Router
{
public:
	Router();
	~Router();

	Router	 &operator=(Router const &Router);
	ABackend *resolve(Request &request, Response &response, async::Reactor &reactor, HTTPStream &http_stream) const;
	void	  addServ(std::string const &domain, RouterNode const &node);
	void	  printServers() const;

private:
	static bool				 validateRequest(Request const &request, Response &response, HTTPStream &http_stream);
	RouterNode const		*getServerNode(std::string const &host, HTTPStream &http_stream) const;
	static RouterNode const *matchSubPath(std::string URI, std::map<std::string, RouterNode> const &sub_nodes);
	static RouterRuleset	 defineRuleset(std::string const &URI, RouterNode const &serv, RouterRuleset &ruleset);
	static ABackend *defineBackend(Request &request, Response &response, RouterRuleset const &ruleset, async::Reactor &reactor, HTTPStream &http_stream, bool force);

	std::map<std::string, RouterNode> _servers; // Servers identified by domain[:port(if other than 80)]
};

#endif
