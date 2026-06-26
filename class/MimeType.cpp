#include "MimeType.hpp"
#include "ConfigParser.hpp"
#include <fcntl.h>
#include <sys/stat.h>

std::map<std::string, std::string> MimeType::mime_type;

void MimeType::setType(std::string const &key, std::string const &value)
{
    std::string rev_key(key.rbegin(), key.rend());
    mime_type[rev_key] = value;
}

// Adds the content of this file to the dictionary, this is called for every mime_type config file
void MimeType::ParseConfig(const char *filepath)
{
    toml::Toml toml_root;
    int        fd = open(filepath, O_RDONLY);
    if (fd < 0)
        throw ConfigParser::ConfigError("Failed to open mime_type config file \"" +
                                        std::string(filepath) +
                                        "\" : " + std::string(strerror(errno)));
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
    for (toml::Array::const_iterator array_it = mime_array.begin(); array_it != mime_array.end();
         array_it++)
    {
        if (array_it->type() != toml::Datum::Table)
            continue;

        const toml::Toml &mime_table = array_it->as_table();

        toml::Toml::const_iterator type_it = mime_table.find("type");
        toml::Toml::const_iterator ext_it  = mime_table.find("extension");

        if (type_it != mime_table.end() && ext_it != mime_table.end() &&
            type_it->second.type() == toml::Datum::String &&
            ext_it->second.type() == toml::Datum::String)
        {
            std::string extension = ext_it->second.as_string();
            if (extension.empty())
                throw ConfigParser::ConfigError("Empty extension");
            std::string type = type_it->second.as_string();
            MimeType::setType(extension, type);
        }
        else
            throw ConfigParser::ConfigError("Missing or invalid 'extension'/'type' keys");
    }
}

std::string MimeType::getType(std::string const &URI)
{
    std::string                                        rev_URI(URI.rbegin(), URI.rend());
    std::map<std::string, std::string>::const_iterator match;
    std::map<std::string, std::string>::const_iterator ite;
    size_t                                             n = 1;

    for (match = mime_type.lower_bound(rev_URI.substr(0, n)); match != ite && n < URI.size();
         match = mime_type.lower_bound(rev_URI.substr(0, ++n)))
    {
        if (match->first.empty())
            return ("");
        if (!rev_URI.compare(0, match->first.size(), match->first))
            return (match->second);
    }
    return ("");
}
