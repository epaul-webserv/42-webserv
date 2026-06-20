#ifndef ROUTERRULESET_HPP
#define ROUTERRULESET_HPP

struct RouterRuleset
{
	RouterRuleset() : list_directories(_false), file_upload(_false), root(DEFAULT_ROOT) {}

	e_switch																	 list_directories;
	e_switch																	 file_upload;
	std::string																	 root;
	std::map<std::string, std::pair<std::string, e_redirect_type> >				 redirects;
	std::map<std::string, std::string>											 status_pages;
	std::vector<std::string>													 index_pages;
	std::map<std::string, std::pair<t_backend_type, std::vector<std::string> > > backend_rules;
	std::vector<std::string>													 accepted_methods;
};

#endif
