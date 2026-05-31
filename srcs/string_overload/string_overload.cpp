#include "string_overload.hpp"

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

std::string to_lower(std::string const &data)
{
	std::string tmp = data;
	std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);

	return tmp;
}

std::string to_upper(std::string const &data)
{
	std::string tmp = data;
	std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);

	return tmp;
}
