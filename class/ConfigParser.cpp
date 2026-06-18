#include "ConfigParser.hpp"
#include <fcntl.h>
#include <unistd.h>

int ConfigParser::ParseConfig(std::map<std::pair<std::string, int>, Router> &config, const char *filepath)
{
	toml::Toml toml;
	int		   fd = open(filepath, O_RDONLY);
	if (fd < 0)
		return (perror("open()"), 1);
	try
	{
		toml::parse(fd, toml);
	}
	catch (const toml::Parser::ParsingError &e)
	{
		std::cerr << "Invalid TOML file: " << e.what() << '\n';
		close(fd);
		return (1);
	}
	close(fd);
	// toml_print(toml, key);
	return (0);
}
