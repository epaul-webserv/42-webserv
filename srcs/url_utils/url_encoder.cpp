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

void encode_char(char c, std::string &dest)
{
    const char base[] = "0123456789ABCDEF";

    if (!to_encode(c))
    {
        dest += c;
        return;
    }
    dest += '%';
    dest += base[c / 16];
    dest += base[c % 16];
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
        encode_char(URI[read], result);
    return (result);
}
