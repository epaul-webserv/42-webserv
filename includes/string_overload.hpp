#ifndef STRING_OVERLOAD_HPP
# define STRING_OVERLOAD_HPP

# include <string>
# include <algorithm>

std::string &to_lower(std::string &data)
{
	std::transform(data.begin(), data.end(), data.begin(), ::tolower);
	return data;
}

std::string &to_upper(std::string &data)
{
	std::transform(data.begin(), data.end(), data.begin(), ::toupper);
	return data;
}

#endif
