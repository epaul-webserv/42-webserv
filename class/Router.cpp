#include "Router.hpp"
#include "ABackend.hpp"
#include "CGIBackend.hpp"
#include "StaticBackend.hpp"
#include "UploadBackend.hpp"
#include "html_generator.hpp"

#include <iostream>
#include <sys/stat.h>
#include <algorithm>

t_file_type Router::fileType(std::string const &path, std::string const &root)
{
    struct stat s = {};

    if (stat((root + path).c_str(), &s) == -1)
    {
        if (errno == EACCES)
            return (_no_access);
        if (errno == ENOENT)
            return (_none);
        return (_err);
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

ABackend *Router::getBackend(t_backend_type const           &backend_type,
                             std::vector<std::string> const &args, std::string const &root,
                             async::Reactor &reactor, HTTPStream &http_stream)
{
    ABackend *backend = NULL;
    (void)args;
    (void)root;
    (void)reactor;
    (void)http_stream;
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
        }
    }
    catch (std::bad_alloc &e)
    {
        std::cerr << "[ERROR] Backend creation has failed : " << e.what() << std::endl;
        http_stream.setHTTPStatus("500", "Internal Server Error");
    }
    return (backend);
}

int Router::getPort() { return _port; }

void Router::setPort(int port) { _port = port; }

bool Router::validateRequest(HTTPStream &http_stream)
{
    const std::string http_version = http_stream.request.http_version;

    if (http_version == "HTTP/1.0" || http_version == "HTTP/1.1")
    {
        http_stream.response.setHTTPVersion(http_version);
        if (http_version == "HTTP/1.1")
        {
            if (http_stream.request.peekHeader("host").empty())
            {
                http_stream.setErrorResponse("400", "Bad Request");
                http_stream.response.addHeader("Connection", "close");
                return (false);
            }
            if (http_stream.request.peekHeader("connection") != "close")
                http_stream.response.addHeader("Connection", "keep-alive");
        }
    }
    else if (!http_version.empty()) // Empty is HTTP/0.9
    {
        http_stream.response.setHTTPVersion("HTTP/1.1");
        http_stream.setErrorResponse("505", "HTTP Version Not Supported");
        return (false);
    }
    if (http_stream.request.URI.empty())
    {
        http_stream.setErrorResponse("400", "Bad Request");
        return (false);
    }
    return (true);
}

RouterNode const *Router::getServerNode(std::string const &host, HTTPStream &http_stream) const
{
    std::map<std::string, RouterNode>::const_iterator it;
    (void)http_stream;

    it = this->_servers.find(host);
    if (it == this->_servers.end())
    {
        it = this->_servers.find("");
        if (it != this->_servers.end())
        {
            std::cout << "Router : fallback to default server" << std::endl;
            return (&it->second);
        }
        http_stream.setErrorResponse("404", "Not Found");
        return (NULL);
    }
    return (&it->second);
}

RouterNode const *Router::matchSubPath(std::string                             &URI,
                                       std::map<std::string, RouterNode> const &sub_nodes)
{
    std::map<std::string, RouterNode>::const_iterator it;
    std::string                                       copy = URI;
    while (!copy.empty())
    {
        it = sub_nodes.find(copy);
        if (it != sub_nodes.end())
        {
            std::cout << it->first.size() << std::endl;
            URI = URI.substr(it->first.size());
            return (&it->second);
        }
        size_t pos = copy.find_last_of('/');
        if (!pos)
            return (NULL);
        copy = copy.substr(0, pos);
    }
    return (NULL);
}

RouterRuleset Router::defineRuleset(std::string &URI, RouterNode const &serv,
                                    RouterRuleset &ruleset)
{
    RouterNode const *matching_sub_node = NULL;

    if (serv.list_directories)
        ruleset.list_directories = serv.list_directories;
    if (serv.file_upload)
        ruleset.file_upload = serv.file_upload;
    if (!serv.method_map.empty())
        ruleset.method_map = serv.method_map;
    if (!serv.root.empty())
        ruleset.root = serv.root;
    ruleset.redirects.insert(serv.redirects.begin(), serv.redirects.end());
    ruleset.status_pages.insert(serv.status_pages.begin(), serv.status_pages.end());
    if (!serv.index_pages.empty())
        ruleset.index_pages = serv.index_pages;
    ruleset.backend_rules.insert(ruleset.backend_rules.end(), serv.backend_rules.begin(),
                                 serv.backend_rules.end());
    if (ruleset.redirects.find(URI) != ruleset.redirects.end())
        return (ruleset);
    if (!serv.sub_nodes.empty())
        matching_sub_node = matchSubPath(URI, serv.sub_nodes);
    if (matching_sub_node)
        return (defineRuleset(URI, *matching_sub_node, ruleset));
    return (ruleset);
}

// static std::vector<std::string>::const_iterator find(std::vector<std::string> const &vec,
//                                                      std::string const              &to_find)
// {
//     std::vector<std::string>::const_iterator it;
//     std::vector<std::string>::const_iterator ite = vec.end();
//
//     for (it = vec.begin(); it != ite; ++it)
//     {
//         if (to_find == *it)
//             return (it);
//     }
//     return (it);
// }

bool    suffix_match(std::string const &URI, std::vector<std::string> const &array)
{
    std::string                                        rev_URI(URI.rbegin(), URI.rend());
    std::vector<std::string>::const_iterator match;
    std::vector<std::string>::const_iterator ite = array.end();
    size_t                                             n = 1;
    std::string                                         cmp;

    for (cmp = rev_URI.substr(0, n); n < URI.size();
         cmp = rev_URI.substr(0, ++n))
    {
        if (std::find(array.begin(), array.end(), cmp) != ite)
            return (true);
    }
    return (false);
}

ABackend *Router::defineBackend(RouterRuleset const &ruleset, async::Reactor &reactor,
                                HTTPStream &http_stream, bool force)
{
    const std::map<std::string, std::pair<std::string, e_redirect_type> >::const_iterator
        matching_redirect = ruleset.redirects.find(http_stream.request.URI);
    if (!force && ruleset.method_map.find(http_stream.request.method) ==
                      ruleset.method_map.end())
    {
        http_stream.setErrorResponse("405", "Method Not Allowed");
        return (NULL);
    }
    if (!force)
    {
        const std::vector<std::string> accepted_filetypes = ruleset.method_map.at(http_stream.request.method);
        if (!accepted_filetypes.empty() && !suffix_match(http_stream.request.rewrited_URI, accepted_filetypes))
        {
            http_stream.setErrorResponse("405", "Method Not Allowed");
            return (NULL);
        }
    }
    if (!force && matching_redirect != ruleset.redirects.end())
    {
        switch (matching_redirect->second.second)
        {
        case _temporary:
            http_stream.setHTTPStatus("307", "Temporary Redirect");
            break;
        case _permanent:
            http_stream.setHTTPStatus("301", "Moved Permanently");
        }
        http_stream.response.addHeader("Location", matching_redirect->second.first);
        return (NULL);
    }
    if (fileType(http_stream.request.rewrited_URI, ruleset.root) == _dir)
    {
        if (!ruleset.index_pages.empty())
        {
            std::vector<std::string>::const_iterator it;
            std::vector<std::string>::const_iterator ite = ruleset.index_pages.end();
            for (it = ruleset.index_pages.begin(); it != ite; ++it)
            {
                if (fileType(http_stream.request.rewrited_URI + "/" + *it, ruleset.root) == _file)
                {
                    http_stream.request.rewrited_URI = "/" + *it;
                    return (getBackend(_static, std::vector<std::string>(), ruleset.root, reactor,
                                       http_stream));
                }
                if (ruleset.list_directories == _false)
                {
                    http_stream.setErrorResponse("404", "Not Found");
                    return (NULL);
                }
            }
        }
        if (ruleset.list_directories == _true)
        {
            return (getBackend(_dir_index, std::vector<std::string>(), ruleset.root, reactor,
                               http_stream));
        }
        http_stream.setErrorResponse("403", "Forbidden");
        return (NULL);
    }
    std::vector<std::pair<std::string, std::pair<t_backend_type, std::vector<std::string> > > >::
        const_iterator it = ruleset.get_corresponding_router(http_stream.request.URI);
    if (it != ruleset.backend_rules.end())
        return (
            getBackend(it->second.first, it->second.second, ruleset.root, reactor, http_stream));
    if (http_stream.request.method == "POST" && ruleset.file_upload == _true)
        return (
            getBackend(_upload, std::vector<std::string>(), ruleset.root, reactor, http_stream));
    return (getBackend(_static, std::vector<std::string>(), ruleset.root, reactor, http_stream));
}

/// @return ABackend pointer OR NULL if response has been set.
ABackend *Router::resolve(async::Reactor &reactor, HTTPStream &http_stream, bool force) const
{
    RouterRuleset ruleset;

    http_stream.response.setHTTPVersion("HTTP/1.1");
    RouterNode const *serv = getServerNode(http_stream.request.peekHeader("host"), http_stream);
    if (!serv)
        return (NULL);
    if (!force && !validateRequest(http_stream))
        return (NULL);
    ruleset = defineRuleset(http_stream.request.rewrited_URI, *serv, ruleset);
    if (force)
        ruleset.root = "";
    http_stream.status_pages = ruleset.status_pages;
    return (defineBackend(ruleset, reactor, http_stream, force));
}

void Router::addServ(std::string const &domain, RouterNode const &node)
{
    this->_servers.insert(std::pair<std::string, RouterNode>(domain, node));
}
