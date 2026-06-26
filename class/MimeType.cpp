#include "MimeType.hpp"

std::map<std::string, std::string> MimeType::mime_type;

void MimeType::setType(std::string const &key, std::string const &value)
{
    std::string rev_key(key.rbegin(), key.rend());
    mime_type[rev_key] = value;
}

#include <iostream>
std::string MimeType::getType(std::string const &URI)
{
    std::string                                        rev_URI(URI.rbegin(), URI.rend());
    std::map<std::string, std::string>::const_iterator match;
    std::map<std::string, std::string>::const_iterator ite;
    size_t                                             n = 1;

    for (match = mime_type.lower_bound(rev_URI.substr(0, n)); match != ite && n < URI.size();
         match = mime_type.lower_bound(rev_URI.substr(0, ++n)))
    {
        if (!rev_URI.compare(0, match->first.size(), match->first))
            return (match->second);
    }
    return ("");
}
