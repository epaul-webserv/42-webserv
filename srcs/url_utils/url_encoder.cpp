#include <cstddef>
#include <string>

bool to_encode(char c)
{
    if (c >= 'A' && c <= 'Z')
        return (false);
    if (c >= 'a' && c <= 'z')
        return (false);
    if (c >= '0' && c <= '9')
        return (false);
    if (c == '-' || c == '.' || c == '_' || c == '~')
        return (false);
    return (true);
}

std::string encode_char(char c)
{
    std::string ret;
    std::string base = "0123456789ABCDEF";

    ret += '%';
    ret += base[c / 16];
    ret += base[c % 16];
    return (ret);
}

/// @brief Translates a URL-encoded string into its ASCII form
/// @param URI Reference to the string to transform.
/// @return The translated ASCII string
std::string encode_url(std::string const &URI)
{
    std::string result;

    if (URI.empty())
        return result;
    for (size_t read = 0; read < URI.size(); ++read)
    {
        if (to_encode(URI[read]))
            result += encode_char(URI[read]);
        else if (URI[read] == '+')
            result += ' ';
        else
            result += URI[read];
    }
    return (result);
}
