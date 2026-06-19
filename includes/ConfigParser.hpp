#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <map>
#include <toml>

#include "Router.hpp"

class ConfigParser
{
public:
	class ConfigError : public std::exception
	{
	protected:
		std::string _msg;

	public:
		explicit ConfigError(const std::string &message) : _msg(message) {}
		virtual ~ConfigError() throw() {}
		virtual const char *what() const throw() { return _msg.c_str(); }
	};

private:
	ConfigParser();
	~ConfigParser();
	ConfigParser(ConfigParser &other);
	ConfigParser &operator=(const ConfigParser &other);

public:
	static void ParseConfig(std::map<struct addrinfo *, Router> &router_map, const char *filepath);
};

#endif
