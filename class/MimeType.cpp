#include "MimeType.hpp"

std::map<std::string, std::string> MimeType::mime_type;

std::string MimeType::getType(std::string const &URI)
{
    std::map<std::string, std::string>::const_iterator it;
    std::map<std::string, std::string>::const_iterator ite = mime_type.end();

    for (it = mime_type.begin(); it != ite; ++it)
    {
        if (!URI.compare(URI.size() - it->first.size(), it->first.size(), it->first))
            return (it->second);
    }
    return ("application/octet-stream");
}
