#ifndef STRING_OVERLOAD_HPP
#define STRING_OVERLOAD_HPP

#include <algorithm>
#include <sstream>
#include <stdint.h>
#include <string>
#include <vector>

std::string &to_lower(std::string &data);

std::string &to_upper(std::string &data);

std::string to_lower(std::string const &data);

std::string to_upper(std::string const &data);

std::vector<uint8_t> &to_lower(std::vector<uint8_t> &data);

std::vector<uint8_t> &to_upper(std::vector<uint8_t> &data);

std::vector<uint8_t> to_lower(std::vector<uint8_t> const &data);

std::vector<uint8_t> to_upper(std::vector<uint8_t> const &data);

template <typename T> std::string toString(const T &value)
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}

#endif
