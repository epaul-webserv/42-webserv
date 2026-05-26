#include <cctype>
#include <cstddef>
#include <string>
#include <cstdlib>

char decodeChar(char high, char low)
{
	char buf[3] = {high, low, '\0'};

	return (static_cast<char>(std::strtol(buf, NULL, 16)));
}

/// @brief Modifies a URL-encoded string into its ASCII form
/// @param URI Reference to the string to transform, the result is written in the same buffer.
void decodeURL(std::string &URI)
{
	size_t wrote = 0;

	if (URI.empty())
		return;
	for (size_t read = 0; read < URI.size(); ++read)
	{
		if (URI[read] == '%' && read + 2 < URI.size())
		{
			if (std::isxdigit(URI[read + 1]) && std::isxdigit(URI[read + 2]))
			{
				URI[wrote++] = decodeChar(URI[read + 1], URI[read + 2]);
				read         += 2;
			}
			else
				URI[wrote++] = URI[++read];
		}
		else if (URI[read] == '+')
			URI[wrote++] = ' ';
		else
			URI[wrote++] = URI[read];
	}
	URI.resize(wrote);
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
				read   += 2;
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
