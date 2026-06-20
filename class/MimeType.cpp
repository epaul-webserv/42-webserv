#include "MimeType.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include "ConfigParser.hpp"

std::map<std::string, std::string> MimeType::mime_type;

bool MimeType::IsValidExtension(const std::string &ext)
{
	if (ext.length() < 2)
		return false;

	if (ext.find_first_of('.') != 0 || ext.find_last_of('.') != 0)
		return false;
	if (ext.find_first_of(" \t\n\r\v\f") != std::string::npos)
		return false;
	return true;
}

// Adds the content of this file to the dictionary, this is called for every mime_type config file
void MimeType::ParseConfig(const char *filepath)
{
	toml::Toml toml_root;
	int		   fd = open(filepath, O_RDONLY);
	if (fd < 0)
		throw ConfigParser::ConfigError("Failed to open mime_type config file \"" + std::string(filepath) + "\" : " + std::string(strerror(errno)));
	try
	{
		toml::parse(fd, toml_root);
	}
	catch (const toml::Parser::ParsingError &e)
	{
		close(fd);
		throw ConfigParser::ConfigError("Invalid TOML file: " + std::string(e.what()));
	}
	close(fd);

	toml::Toml::const_iterator it = toml_root.find("mime_types");
	if (it == toml_root.end() || it->second.type() != toml::Datum::Array)
		throw ConfigParser::ConfigError("Missing or invalid 'mime_types' array");

	const toml::Array &mime_array = it->second.as_array();
	for (toml::Array::const_iterator array_it = mime_array.begin(); array_it != mime_array.end(); array_it++)
	{
		if (array_it->type() != toml::Datum::Table)
			continue;

		const toml::Toml &mime_table = array_it->as_table();

		toml::Toml::const_iterator type_it = mime_table.find("type");
		toml::Toml::const_iterator ext_it  = mime_table.find("extension");

		if (type_it != mime_table.end() && ext_it != mime_table.end() && type_it->second.type() == toml::Datum::String && ext_it->second.type() == toml::Datum::String)
		{
			std::string extension = ext_it->second.as_string();
			if (!IsValidExtension(extension))
				throw ConfigParser::ConfigError("Bad extension format: " + extension);
			std::string type			   = type_it->second.as_string();
			MimeType::mime_type[extension] = type;
		}
		else
			throw ConfigParser::ConfigError("Missing or invalid 'extension'/'type' keys");
	}
}
