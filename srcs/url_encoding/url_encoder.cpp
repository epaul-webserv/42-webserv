#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <string>

std::string encodeChar(char high, char low)
{
	char buf[3] = {high, low, '\0'};

	return (static_cast<char>(std::strtol(buf, NULL, 16)));
}

/// @brief Traslates a URL-encoded string into its ASCII form
/// @param URI Reference to the string to transform.
/// @return The translated ASCII string
std::string decodeURL(std::string const &URI)
{
	std::string result;

	if (URI.empty())
		return result;
	for (size_t read = 0; read < URI.size(); ++read)
	{
		if (URI[read] == '%' && read + 2 < URI.size())
		{
			if (std::isxdigit(URI[read + 1]) && std::isxdigit(URI[read + 2]))
			{
				result += decodeChar(URI[read + 1], URI[read + 2]);
				read += 2;
			}
			else
				result += URI[++read];
		}
		else if (URI[read] == '+')
			result += ' ';
		else
			result += URI[read];
	}
	return (result);
}
