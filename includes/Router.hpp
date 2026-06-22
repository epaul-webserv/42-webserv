#ifndef ROUTER_H
#define ROUTER_H

#include "ABackend.hpp"
#include "HTTPStream.hpp"
#include "Response.hpp"
#include "RouterNode.hpp"
#include "RouterRuleset.hpp"
#include "url_utils.hpp"
#include <async>
#include <cerrno>
#include <vector>

class HTTPStream;

class Router
{
  public:
    Router();
    ~Router();

    Router          &operator=(Router const &Router);
    ABackend        *resolve(Request &request, Response &response, async::Reactor &reactor,
                             HTTPStream &http_stream, bool force) const;
    void             addServ(std::string const &domain, RouterNode const &node);
    void             printServers() const;
    static ABackend *getBackend(t_backend_type const           &backend_type,
                                std::vector<std::string> const &args, std::string const &root,
                                async::Reactor &reactor, HTTPStream &http_stream);
    int              getPort();
    void             setPort(int port);

  private:
    static bool              validateRequest(Request const &request, Response &response,
                                             HTTPStream &http_stream);
    RouterNode const        *getServerNode(std::string const &host, HTTPStream &http_stream) const;
    static RouterNode const *matchSubPath(std::string                              URI,
                                          std::map<std::string, RouterNode> const &sub_nodes);
    static RouterRuleset     defineRuleset(std::string const &URI, RouterNode const &serv,
                                           RouterRuleset &ruleset);
    static ABackend         *defineBackend(Request &request, Response &response,
                                           RouterRuleset const &ruleset, async::Reactor &reactor,
                                           HTTPStream &http_stream, bool force);

    std::map<std::string, RouterNode>
        _servers; // Servers identified by domain[:port(if other than 80)]
    int _port;
};

#endif
