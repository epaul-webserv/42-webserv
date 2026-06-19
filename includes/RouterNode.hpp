#ifndef ROUTERNODE_HPP
#define ROUTERNODE_HPP

#include <map>
#include <string>
#include <vector>
#include "ABackend.hpp"

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
	RouterNode() : list_directories(_false), file_upload(_false), location("/"), root(DEFAULT_ROOT) {}
	RouterNode(std::string const &location) : list_directories(_false), file_upload(_false), location(location) {}

	e_switch														list_directories;
	e_switch														file_upload;
	std::string const												location;
	std::string														root; // path to the root of the files to serve, inactive if empty
	std::map<std::string, RouterNode>								sub_nodes; // relative (to active node URI) URI of a sub path, leading to another rule subset
	std::map<std::string, std::pair<std::string, e_redirect_type> > redirects; // URI to redirect, new location and type of redirection
	std::map<std::string, std::string> status_pages; // status, relative (to applicable root) path of page to serve in case of this status;
	std::string						   dir_file; // relative (to applicable root) path to the file to serve in case on a dir ressource, inactive if empty
	std::map<std::string, std::pair<t_backend_type, std::vector<std::string> > >
		backend_rules; // List of accessible backends (file extension, backend type and backend arguments first one being the absolute path to the executable if
	// applicable)
	std::vector<std::string> accepted_methods;
	std::vector<std::string> index_pages;
	// FIXME: ADD DEFAULT INDEX PAGES
};

#endif
