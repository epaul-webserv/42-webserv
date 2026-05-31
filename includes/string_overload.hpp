#ifndef STRING_OVERLOAD_HPP
#define STRING_OVERLOAD_HPP

#include <algorithm>
#include <string>

std::string &to_lower(std::string &data);

std::string &to_upper(std::string &data);

std::string to_lower(std::string const &data);

std::string to_upper(std::string const &data);

#endif
