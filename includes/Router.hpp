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
    static t_file_type fileType(std::string const &path, std::string const &root);
    Router();
    ~Router();

    Router          &operator=(Router const &Router);
    ABackend        *resolve(async::Reactor &reactor, HTTPStream &http_stream, bool force) const;
    void             addServ(std::string const &domain, RouterNode const &node);
    static ABackend *getBackend(t_backend_type const           &backend_type,
                                std::vector<std::string> const &args, std::string const &root,
                                async::Reactor &reactor, HTTPStream &http_stream);
    int              getPort() const;
    void             setPort(int port);

  private:
    static bool       validateRequest(HTTPStream &http_stream);
    RouterNode const *getServerNode(std::string const &host, HTTPStream &http_stream) const;
    static bool       matchSubPath(std::string const                            &URI,
                                   std::multimap<std::string, RouterNode>       &matching_nodes,
                                   std::multimap<std::string, RouterNode> const &sub_nodes);
    static RouterNode const                       *
    matchSubPath_2(std::string &URI, std::string const &method,
                                         std::multimap<std::string, RouterNode> const &matching_nodes);
    static RouterRuleset *defineRuleset(HTTPStream &http_stream, RouterNode const &serv,
                                        RouterRuleset &ruleset);
    static ABackend      *defineBackend(RouterRuleset const &ruleset, async::Reactor &reactor,
                                        HTTPStream &http_stream, bool force);

    std::map<std::string, RouterNode>
        _servers; // Servers identified by domain[:port(if other than 80)]
    int _port;
};

#endif
