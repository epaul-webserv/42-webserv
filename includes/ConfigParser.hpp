#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <map>
#include <toml>

#include "Router.hpp"

class ConfigParser
{
private:
	ConfigParser();
	~ConfigParser();
	ConfigParser(ConfigParser &other);
	ConfigParser &operator=(const ConfigParser &other);

public:
	static int ParseConfig(std::map<std::pair<std::string, int>, Router> &config, const char *filepath) throw(toml::Parser::ParsingError);
};

#endif
