#include "Router.hpp"
#include "ABackend.hpp"
#include <iostream>
#include <sys/stat.h>

static t_file_type fileType(std::string const &path, std::string const &root)
{
    struct stat s = {};

    if (stat((root + path).c_str(), &s) == -1)
    {
        if (errno == EACCES)
            return (_no_access);
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
            std::cout << "Router : Would respond with static backend" << std::endl;
            // backend = new StaticBackend(root, http_stream);
            break;
        case _cgi:
            std::cout << "Router : Would respond with cgi backend" << std::endl;
            // backend = new CGIBackend(args, root, reactor, http_stream);
            break;
        case _dir_index:
            std::cout << "Router : Would respond with dir backend" << std::endl;
            // backend = new StaticBackend(root, http_stream);
            break;
        case _upload:
            std::cout << "Router : Would respond with upload backend" << std::endl;
            // backend = new UploadBackend(root, http_stream, http_stream.request_body());
            break;
        }
    }
    catch (std::bad_alloc &e)
    {
        std::cerr << "[ERROR] Backend creation has failed : " << e.what() << std::endl;
        // http_stream.setHTTPStatus("500", "Internal Server Error");
    }
    return (backend);
}

int Router::getPort() { return _port; }

void Router::setPort(int port) { _port = port; }

bool Router::validateRequest(HTTPStream &http_stream)
{
    const std::string http_version = http_stream.request.getHTTPVersion();

    if (http_version == "HTTP/1.0" || http_version == "HTTP/1.1")
    {
        std::cout << "Router : Setting http version to " << http_version << std::endl;
        // response.setHTTPVersion(http_version);
        if (http_version == "HTTP/1.1")
        {
            if (http_stream.request.peekHeader("host").empty())
            {
                std::cout << "Router : No Host header found (mandatory for HTTP/1.1) - Error 400"
                          << std::endl;
                // http_stream.setHTTPStatus("400", "Bad Request");
                // response.addHeader("Connection", "close");
                return (false);
            }
            if (http_stream.request.peekHeader("connection") != "close")
            {
                // response.addHeader("Connection", "keep-alive");
            }
        }
    }
    else if (!http_version.empty()) // Empty is HTTP/0.9
    {
        std::cout << "Router : Unrecognized http version, fall back to 1.1 " << std::endl;
        // response.setHTTPVersion("HTTP/1.1");
        std::cout << "Router : http version not recognized - Error 505" << std::endl;
        // http_stream.setHTTPStatus("505", "HTTP Version Not Supported");
        return (false);
    }
    if (http_stream.request.getURI().empty())
    {
        std::cout << "Router : URI is empty - Error 400" << std::endl;
        // http_stream.setHTTPStatus("400", "Bad Request");
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
        std::cout
            << "Router : No corresponding server found (and no default server set) : error 404 "
            << std::endl;
        // http_stream.setHTTPStatus("404", "Not Found");
        return (NULL);
    }
    return (&it->second);
}

RouterNode const *Router::matchSubPath(std::string                              URI,
                                       std::map<std::string, RouterNode> const &sub_nodes)
{
    std::map<std::string, RouterNode>::const_iterator it;
    while (!URI.empty())
    {
        URI = URI.substr(0, URI.find_last_of('/') - 1);
        it  = sub_nodes.find(URI);
        if (it != sub_nodes.end())
            return (&it->second);
    }
    return (NULL);
}

RouterRuleset Router::defineRuleset(std::string const &URI, RouterNode const &serv,
                                    RouterRuleset &ruleset)
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

static std::vector<std::string>::const_iterator find(std::vector<std::string> const &vec,
                                                     std::string const              &to_find)
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

ABackend *Router::defineBackend(RouterRuleset const &ruleset, async::Reactor &reactor,
                                HTTPStream &http_stream, bool force)
{
    const std::map<std::string, std::pair<std::string, e_redirect_type> >::const_iterator
        matching_redirect = ruleset.redirects.find(http_stream.request.getURI());
    if (!force && find(ruleset.accepted_methods, http_stream.request.getMethod()) ==
                      ruleset.accepted_methods.end())
    {
        std::cout << "Router : Method not accepted - Error 405" << std::endl;
        // http_stream.setHTTPStatus("405", "Method Not Allowed");
        return (NULL);
    }
    if (!force && matching_redirect != ruleset.redirects.end())
    {
        switch (matching_redirect->second.second)
        {
        case _temporary:
            std::cout << "Router : Temporary redirect to " << matching_redirect->second.first
                      << std::endl;
            // http_stream.setHTTPStatus("307", "Temporary Redirect");
            // response.bodyAppend(get_html_error("307", "Temporary Redirect"));
            break;
        case _permanent:
            std::cout << "Router : Permanent redirect to " << matching_redirect->second.first
                      << std::endl;
            // http_stream.setHTTPStatus("301", "Moved Permanently");
            // response.bodyAppend(get_html_error("301", "Moved Permanently"));
        }
        // response.addHeader("Location", matching_redirect->second.first);
        return (NULL);
    }
    if (fileType(http_stream.request.getURI(), ruleset.root) == _dir)
    {
        if (!ruleset.index_pages.empty())
        {
            std::vector<std::string>::const_iterator it;
            std::vector<std::string>::const_iterator ite = ruleset.index_pages.end();
            for (it = ruleset.index_pages.begin(); it != ite; ++it)
            {
                if (fileType("/" + *it, ruleset.root) == _file)
                {
                    http_stream.request.setURI("/" + *it);
                    std::cout << "Router : Index page matched : " << *it << std::endl;
                    return (NULL);
                    // return (getBackend(_static, std::vector<std::string>(), ruleset.root,
                    // reactor,
                    //                    http_stream));
                }
            }
        }
        if (ruleset.list_directories == _true)
        {
            std::cout << "Router : Would serve generated directory listing with static backend"
                      << std::endl;
            return (NULL);
            // return (getBackend(_dir_index, std::vector<std::string>(), ruleset.root, reactor,
            //                    http_stream));
        }
        std::cout << "Router : No good response for directory request - Error 403" << std::endl;
        // http_stream.setHTTPStatus("403", "Forbidden");
        return (NULL);
    }
    std::vector<std::pair<std::string, std::pair<t_backend_type, std::vector<std::string> > > >::
        const_iterator it = ruleset.get_corresponding_router(http_stream.request.getURI());
    if (it != ruleset.backend_rules.end())
        return (
            getBackend(it->second.first, it->second.second, ruleset.root, reactor, http_stream));
    if (http_stream.request.getMethod() == "POST" && ruleset.file_upload == _true)
        return (
            getBackend(_upload, std::vector<std::string>(), ruleset.root, reactor, http_stream));
    return (getBackend(_static, std::vector<std::string>(), ruleset.root, reactor, http_stream));
}

/// @return ABackend pointer OR NULL if response has been set.
ABackend *Router::resolve(async::Reactor &reactor, HTTPStream &http_stream, bool force) const
{
    RouterRuleset ruleset;

    // response.setHTTPVersion("HTTP/1.1");
    RouterNode const *serv = getServerNode(http_stream.request.peekHeader("host"), http_stream);
    if (!serv)
        return (NULL);
    if (!force && !validateRequest(http_stream))
        return (NULL);
    ruleset = defineRuleset(http_stream.request.getURI(), *serv, ruleset);
    // http_stream.status_pages = ruleset.status_pages;
    return (defineBackend(ruleset, reactor, http_stream, force));
}

void Router::addServ(std::string const &domain, RouterNode const &node)
{
    this->_servers.insert(std::pair<std::string, RouterNode>(domain, node));
}

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
        for (std::map<std::string, std::string>::const_iterator it = node.status_pages.begin();
             it != node.status_pages.end(); ++it)
        {
            std::cout << indent << "    " << it->first << " -> " << it->second << "\n";
        }
    }

    // Print Redirects
    if (!node.redirects.empty())
    {
        std::cout << indent << "  Redirects:\n";
        for (std::map<std::string, std::pair<std::string, e_redirect_type> >::const_iterator it =
                 node.redirects.begin();
             it != node.redirects.end(); ++it)
        {
            std::cout << indent << "    " << it->first << " -> " << it->second.first;
            std::cout << " (" << (it->second.second == _permanent ? "Permanent" : "Temporary")
                      << ")\n";
        }
    }

    // Print Backend Rules
    if (!node.backend_rules.empty())
    {
        std::cout << indent << "  Backend Rules:\n";
        for (std::vector<
                 std::pair<std::string, std::pair<t_backend_type, std::vector<std::string> > > >::
                 const_iterator it = node.backend_rules.begin();
             it != node.backend_rules.end(); ++it)
        {
            std::cout << indent << "    Extension: ." << it->first << "\n";
            std::cout << indent << "      Type: " << (it->second.first == _cgi ? "CGI" : "Unknown")
                      << "\n";
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
        for (std::map<std::string, RouterNode>::const_iterator it = node.sub_nodes.begin();
             it != node.sub_nodes.end(); ++it)
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

    for (std::map<std::string, RouterNode>::const_iterator it = _servers.begin();
         it != _servers.end(); ++it)
    {
        std::cout << "  Server Name (Key): \"" << it->first << "\"\n";
        printNode(it->second, "    ");
        std::cout << "  -------------------------------------\n";
    }
}
