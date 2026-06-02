#ifndef URL_UTILS_H
#define URL_UTILS_H

void		decodeURL(std::string &URI);
std::string decodeURL(std::string const &URI);
std::string encodeURL(std::string const &URI);
std::string get_file_ext(std::string const &URI);
std::string get_file_name(std::string const &URI);

#endif
