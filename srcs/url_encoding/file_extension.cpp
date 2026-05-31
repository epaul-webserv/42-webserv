#include <string>

std::string get_file_ext(std::string const &URI)
{
	std::string tmp = URI;
	if (tmp.find('?') != std::string::npos)
		tmp.resize(tmp.find_first_of('?'));
	if (tmp.find('.') != std::string::npos)
		tmp = tmp.substr(tmp.find_last_of('.') + 1, tmp.size());
	return (tmp);
}
