#ifndef URL_UTILS_H
#define URL_UTILS_H

#include <string>

void        decode_url(std::string &URI);
std::string decode_url(std::string const &URI);
std::string encode_url(std::string const &URI);
std::string get_file_name(std::string const &URI);

#endif
