#include "ConfigParser.hpp"
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cctype>
#include <string>

bool containsWhitespace(const std::string &str)
{
    for (size_t i = 0; i < str.length(); ++i)
    {
        if (std::isspace(static_cast<unsigned char>(str[i])))
            return true;
    }
    return false;
}

bool is_absolute(const std::string &str) { return !str.empty() && str.at(0) == '/'; }

bool has_trailing_slash(const std::string &str)
{
    return !str.empty() && str.at(str.length() - 1) == '/';
}

// Helper to convert boolean to custom e_switch enum
static e_switch get_switch_value(bool val) { return val ? _true : _false; }

// Helper to convert redirect string to e_redirect_type enum
static e_redirect_type get_redirect_type(const std::string &type)
{
    if (type == "perm")
        return _permanent;
    else if (type == "temp")
        return _temporary;
    throw ConfigParser::ConfigError("Wrong redirection type: " + type);
}

// Helper to match backend type string to enum
static t_backend_type get_backend_type(const std::string &type)
{
    if (type == "cgi")
        return _cgi;
    if (type == "static")
        return _static;
    if (type == "dir_index")
        return _dir_index;
    if (type == "upload")
        return _upload;
    throw ConfigParser::ConfigError("Wrong backend type: " + type);
}

// Helper to get addrinfo* from domain and port
static struct addrinfo *resolve_listen(const std::string &domain, const std::string &port)
{
    struct addrinfo  hints  = {};
    struct addrinfo *result = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family    = AF_UNSPEC;
    hints.ai_socktype  = SOCK_STREAM;
    hints.ai_flags     = 0;
    hints.ai_protocol  = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr      = NULL;
    hints.ai_next      = NULL;
    int s              = getaddrinfo(domain.c_str(), port.c_str(), &hints, &result);
    if (s != 0)
    {
        std::cerr << "getaddrinfo error for " << domain << ":" << port << " -> " << gai_strerror(s)
                  << "\n";
        return NULL;
    }
    return result;
}

// Cleans up dynamically allocated addrinfo resources if an error occurs mid-parsing
static void free_router_map_keys(std::map<struct addrinfo *, Router> &router_map)
{
    for (std::map<struct addrinfo *, Router>::iterator it = router_map.begin();
         it != router_map.end(); ++it)
    {
        if (it->first != NULL)
        {
            freeaddrinfo(it->first);
        }
    }
    router_map.clear();
}

// Recursive helper function to extract a RouterNode and its nested sub-locations
static RouterNode parseLocationNode(const toml::Toml &loc_table)
{
    std::string                path    = "/";
    toml::Toml::const_iterator path_it = loc_table.find("path");
    if (path_it != loc_table.end())
    {
        path = path_it->second.as_string();
        if (!is_absolute(path))
            throw ConfigParser::ConfigError("location path must start with \"/\"");
    }

    RouterNode loc_node(path);

    // root path configuration
    toml::Toml::const_iterator root_it = loc_table.find("root");
    if (root_it != loc_table.end())
    {
        if (!is_absolute(root_it->second.as_string()))
            throw ConfigParser::ConfigError("root must be an absolute path");
        if (has_trailing_slash(root_it->second.as_string()))
            throw ConfigParser::ConfigError("Root cannot end with \"/\"");
        loc_node.root = root_it->second.as_string();
    }
    else
        throw ConfigParser::ConfigError("root key missing in RouterNode");

    // flags conversion to e_switch
    toml::Toml::const_iterator dir_it = loc_table.find("directory_listing");
    if (dir_it != loc_table.end())
    {
        loc_node.list_directories = get_switch_value(dir_it->second.as_bool());
    }
    else
        loc_node.list_directories = _inherit;

    toml::Toml::const_iterator up_it = loc_table.find("file_upload");
    if (up_it != loc_table.end())
    {
        loc_node.file_upload = get_switch_value(up_it->second.as_bool());
    }
    else
        loc_node.file_upload = _inherit;

    toml::Toml::const_iterator siz_it = loc_table.find("max_resource_size");
    if (siz_it != loc_table.end())
    {
        loc_node.max_resource_size = siz_it->second.as_int64();
        if (loc_node.max_resource_size < -1)
            throw ConfigParser::ConfigError(
                "Max Resource size must be greater or equal to 0, it can be set to -1 or left "
                "unspecified for unrestricted resource sized");
    }
    else
        loc_node.max_resource_size = -2;

    toml::Toml::const_iterator map_lookup = loc_table.find("method_map");
    if (map_lookup != loc_table.end() && map_lookup->second.type() == toml::Datum::Array)
    {
        const toml::Array &map_arr = map_lookup->second.as_array();
        for (toml::Array::const_iterator m_it = map_arr.begin(); m_it != map_arr.end(); ++m_it)
        {
            if (m_it->type() == toml::Datum::Table)
            {
                const toml::Toml &inner_table = m_it->as_table();

                // Iterate through the key-value pairs in each table (e.g., GET = [...])
                for (toml::Toml::const_iterator t_it = inner_table.begin();
                     t_it != inner_table.end(); ++t_it)
                {
                    std::string method_name = t_it->first;
                    if (containsWhitespace(method_name))
                        throw ConfigParser::ConfigError(
                            "Method names in method_map cannot contain whitespace");

                    std::vector<std::string> extensions;
                    if (t_it->second.type() == toml::Datum::Array)
                    {
                        const toml::Array &ext_arr = t_it->second.as_array();
                        for (toml::Array::const_iterator e_it = ext_arr.begin();
                             e_it != ext_arr.end(); ++e_it)
                        {
                            std::string ext = e_it->as_string();
                            extensions.push_back(std::string(ext.rbegin(), ext.rend()));
                        }
                    }
                    else
                    {
                        throw ConfigParser::ConfigError("Value for method " + method_name +
                                                        " in method_map must be an array");
                    }

                    loc_node.method_map[method_name] = extensions;
                }
            }
        }
    }

    // default index pages
    toml::Toml::const_iterator index_lookup = loc_table.find("index");
    if (index_lookup != loc_table.end() && index_lookup->second.type() == toml::Datum::Array)
    {
        const toml::Array &indices = index_lookup->second.as_array();
        for (toml::Array::const_iterator i_it = indices.begin(); i_it != indices.end(); ++i_it)
        {
            // TODO: does this need any rule ?
            loc_node.index_pages.push_back(i_it->as_string());
        }
    }

    // custom error target status pages
    toml::Toml::const_iterator status_lookup = loc_table.find("status_pages");
    if (status_lookup != loc_table.end() && status_lookup->second.type() == toml::Datum::Array)
    {
        const toml::Array &pages = status_lookup->second.as_array();
        for (toml::Array::const_iterator p_it = pages.begin(); p_it != pages.end(); ++p_it)
        {
            if (p_it->type() == toml::Datum::Table)
            {
                const toml::Toml          &page_table = p_it->as_table();
                toml::Toml::const_iterator st_code    = page_table.find("status");
                toml::Toml::const_iterator st_targ    = page_table.find("target");

                if (st_code != page_table.end() && st_targ != page_table.end())
                {
                    std::stringstream ss;
                    ss << st_code->second.as_int64();
                    if (!is_absolute(st_targ->second.as_string()))
                        throw ConfigParser::ConfigError(
                            "Status page target must be an absolute path");
                    loc_node.status_pages[ss.str()] = st_targ->second.as_string();
                }
            }
        }
    }

    // rule redirections
    toml::Toml::const_iterator redir_lookup = loc_table.find("redirects");
    if (redir_lookup != loc_table.end() && redir_lookup->second.type() == toml::Datum::Array)
    {
        const toml::Array &redirects_arr = redir_lookup->second.as_array();
        for (toml::Array::const_iterator r_it = redirects_arr.begin(); r_it != redirects_arr.end();
             ++r_it)
        {
            if (r_it->type() == toml::Datum::Table)
            {
                const toml::Toml          &red_table = r_it->as_table();
                toml::Toml::const_iterator r_path    = red_table.find("path");
                toml::Toml::const_iterator r_targ    = red_table.find("target");
                toml::Toml::const_iterator r_type    = red_table.find("type");

                if (r_path != red_table.end() && r_targ != red_table.end() &&
                    r_type != red_table.end())
                {
                    if (!is_absolute(r_path->second.as_string()))
                        throw ConfigParser::ConfigError("Redirection path must be absolute");
                    std::string     path_str   = r_path->second.as_string();
                    std::string     target_str = r_targ->second.as_string();
                    e_redirect_type type_enum  = get_redirect_type(r_type->second.as_string());

                    loc_node.redirects[path_str] =
                        std::pair<std::string, e_redirect_type>(target_str, type_enum);
                }
            }
        }
    }

    // backends & interpreters extraction
    toml::Toml::const_iterator back_lookup = loc_table.find("backends");
    if (back_lookup != loc_table.end() && back_lookup->second.type() == toml::Datum::Array)
    {
        const toml::Array &backends_arr = back_lookup->second.as_array();
        for (toml::Array::const_iterator b_it = backends_arr.begin(); b_it != backends_arr.end();
             ++b_it)
        {
            if (b_it->type() == toml::Datum::Array)
            {
                const toml::Array &b_rules = b_it->as_array();
                if (b_rules.size() == 3 || b_rules.size() == 2)
                {
                    toml::Array::const_iterator rule_it = b_rules.begin();

                    std::string extension = rule_it->as_string();
                    ++rule_it;

                    t_backend_type b_type = get_backend_type(rule_it->as_string());

                    std::vector<std::string> b_args;
                    if (b_type == _cgi && b_rules.size() == 3)
                    {
                        ++rule_it;
                        if (rule_it->type() == toml::Datum::Array)
                        {
                            const toml::Array &args_arr = rule_it->as_array();
                            for (toml::Array::const_iterator a_it = args_arr.begin();
                                 a_it != args_arr.end(); ++a_it)
                            {
                                b_args.push_back(a_it->as_string());
                            }
                            if (b_args.size() == 0)
                                ConfigParser::ConfigError(
                                    "cgi backend arguments are empty, it should at least contain a "
                                    "path to the executable");
                        }
                    }
                    else
                        throw ConfigParser::ConfigError("missing arguemnts to configure CGI for ." +
                                                        extension);
                    loc_node.backend_rules.push_back(
                        std::pair<std::string,
                                  std::pair<t_backend_type, std::vector<std::string> > >(
                            extension,
                            std::pair<t_backend_type, std::vector<std::string> >(b_type, b_args)));
                }
                else
                    throw ConfigParser::ConfigError("Wrong number of arguments for backend config");
            }
        }
    }

    // RECURSIVE LOOKUP FOR NESTED LOCATIONS: (Note: server_name is completely ignored here)
    toml::Toml::const_iterator sub_loc_lookup = loc_table.find("location");
    if (sub_loc_lookup != loc_table.end() && sub_loc_lookup->second.type() == toml::Datum::Array)
    {
        const toml::Array &sub_locations_array = sub_loc_lookup->second.as_array();
        for (toml::Array::const_iterator sl_it = sub_locations_array.begin();
             sl_it != sub_locations_array.end(); ++sl_it)
        {
            if (sl_it->type() != toml::Datum::Table)
                continue;

            RouterNode child_node = parseLocationNode(sl_it->as_table());
            loc_node.sub_nodes.insert(
                std::pair<std::string, RouterNode>(child_node.location, child_node));
        }
    }

    return loc_node;
}
#include "MimeType.hpp"

void ConfigParser::ParseConfig(std::map<struct addrinfo *, Router> &router_map,
                               const char                          *filepath)
{
    toml::Toml toml_root;
    int        fd = open(filepath, O_RDONLY);
    if (fd < 0)
        throw ConfigParser::ConfigError("Failed to open config file \"" + std::string(filepath) +
                                        "\" : " + std::string(strerror(errno)));
    try
    {
        toml::parse(fd, toml_root);
    }
    catch (const toml::Parser::ParsingError &e)
    {
        throw ConfigParser::ConfigError("Invalid TOML file: " + std::string(e.what()));
    }

    toml::Toml::const_iterator mime_lookup = toml_root.find("mime_config");
    if (mime_lookup != toml_root.end())
    {
        if (mime_lookup->second.type() == toml::Datum::Array)
        {
            const toml::Array &arr = mime_lookup->second.as_array();
            for (toml::Array::const_iterator it = arr.begin(); it != arr.end(); ++it)
            {
                if (it->type() == toml::Datum::String)
                    MimeType::ParseConfig(it->as_string().c_str());
            }
        }
    }

    // Look up the listeners (handles both "listeners" and "listener" to match your TOML robustly)
    toml::Toml::const_iterator listeners_lookup = toml_root.find("listeners");
    if (listeners_lookup == toml_root.end())
    {
        throw ConfigParser::ConfigError("Config missing [listeners] block");
    }

    // Prepare to iterate through listeners whether it is a single table or an array of tables
    std::vector<toml::Toml> listeners_vector;
    if (listeners_lookup->second.type() == toml::Datum::Array)
    {
        const toml::Array &arr = listeners_lookup->second.as_array();
        for (toml::Array::const_iterator it = arr.begin(); it != arr.end(); ++it)
        {
            if (it->type() == toml::Datum::Table)
                listeners_vector.push_back(it->as_table());
        }
    }
    else if (listeners_lookup->second.type() == toml::Datum::Table)
    {
        listeners_vector.push_back(listeners_lookup->second.as_table());
    }

    // Iterate through every listener block
    for (std::vector<toml::Toml>::const_iterator lit = listeners_vector.begin();
         lit != listeners_vector.end(); ++lit)
    {
        const toml::Toml &listener_table = *lit;
        std::string       domain         = "localhost";
        std::string       port           = "80";
        int64_t           port_nb        = 80;

        // Listen block
        toml::Toml::const_iterator listen_lookup = listener_table.find("listen");
        if (listen_lookup != listener_table.end() &&
            listen_lookup->second.type() == toml::Datum::Table)
        {
            const toml::Toml &listen_table = listen_lookup->second.as_table();

            toml::Toml::const_iterator dom_it = listen_table.find("domain");
            if (dom_it != listen_table.end())
            {
                domain = dom_it->second.as_string();
            }
            else
                throw ConfigParser::ConfigError("missing domain");

            toml::Toml::const_iterator port_it = listen_table.find("port");
            if (port_it != listen_table.end())
            {
                std::stringstream ss;
                port_nb = port_it->second.as_int64();
                if (port_nb < 0 || port_nb > 65535)
                {
                    free_router_map_keys(router_map);
                    throw ConfigParser::ConfigError("port must be in 0-65535 range");
                }
                ss << port_nb;
                port = ss.str();
            }
            else
                throw ConfigParser::ConfigError("missing port");
        }
        else
            throw ConfigParser::ConfigError("listen key missing in listener block");

        // Get addrinfo socket configuration block
        struct addrinfo *ai = resolve_listen(domain, port);
        if (ai == NULL)
        {
            free_router_map_keys(router_map);
            throw ConfigParser::ConfigError("failed to get addrinfo");
        }

        Router &router = router_map[ai];

        toml::Toml::const_iterator server_lookup = listener_table.find("server");
        if (server_lookup != listener_table.end())
        {
            if (server_lookup->second.type() != toml::Datum::Array)
                throw ConfigParser::ConfigError("server block must be an array of tables");

            const toml::Array &server_array = server_lookup->second.as_array();

            toml::Array::const_iterator server_it = server_array.begin();
            if (server_it == server_array.end())
                throw ConfigParser::ConfigError("Empty Server block");
            while (server_it != server_array.end())
            {
                if (server_it->type() != toml::Datum::Table)
                    throw ConfigParser::ConfigError(
                        "Each element in the server array must be a table");

                const toml::Toml &server_table = server_it->as_table();

                std::string                server_name_key = "";
                toml::Toml::const_iterator name_it         = server_table.find("server_name");
                if (name_it != server_table.end())
                {
                    server_name_key = name_it->second.as_string();
                }
                RouterNode server_root_node = parseLocationNode(server_table);
                router.addServ(server_name_key, server_root_node);
                server_it++;
            }
        }
        else
        {
            throw ConfigParser::ConfigError("Missing server block");
        }
    }
}
