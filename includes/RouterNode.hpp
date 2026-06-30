#ifndef ROUTERNODE_HPP
#define ROUTERNODE_HPP

#include "ABackend.hpp"
#include "webserv.hpp"
#include <map>
#include <string>
#include <vector>

enum e_redirect_type
{
    _permanent,
    _temporary
};

enum e_switch
{
    _inherit,
    _true,
    _false
};

struct RouterNode
{
    RouterNode()
        : list_directories(_inherit), file_upload(_inherit), location("/"), root(DEFAULT_ROOT),
          max_resource_size(-1)
    {
    }
    RouterNode(std::string const &location)
        : list_directories(_inherit), file_upload(_inherit), location(location)
    {
    }
    RouterNode &operator=(const RouterNode &other)
    {
        if (this != &other)
        {
            list_directories  = other.list_directories;
            file_upload       = other.file_upload;
            location          = other.location;
            root              = other.root;
            sub_nodes         = other.sub_nodes;
            redirects         = other.redirects;
            status_pages      = other.status_pages;
            index_pages       = other.index_pages;
            backend_rules     = other.backend_rules;
            method_map        = other.method_map;
            max_resource_size = other.max_resource_size;
        }
        return (*this);
    }

    e_switch    list_directories;
    e_switch    file_upload;
    std::string location;
    std::string root; // path to the root of the files to serve, inactive if empty
    std::map<std::string, RouterNode> sub_nodes; // relative (to active node URI) URI of a sub path,
                                                 // leading to another rule subset
    std::map<std::string, std::pair<std::string, e_redirect_type> >
        redirects; // URI to redirect, new location and type of redirection
    std::map<std::string, std::string> status_pages; // status, relative (to applicable root) path
                                                     // of page to serve in case of this status;
    std::vector<std::string> index_pages; // relative (to applicable root) path to the file to serve
                                          // in case on a dir resource, inactive if empty
    std::vector<std::pair<std::string, std::pair<t_backend_type, std::vector<std::string> > > >
        backend_rules; // List of accessible backends (file extension, backend type and backend
                       // arguments first one being the absolute path to the executable if
    // applicable)
    std::map<std::string, std::vector<std::string> > method_map;
    ssize_t                                          max_resource_size;
};

#endif
