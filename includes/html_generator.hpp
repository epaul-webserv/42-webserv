#ifndef HTML_GENERATOR_HPP
#define HTML_GENERATOR_HPP

#include <cstring>
#include <ctime>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include "Date.hpp"
#include "FdStream.hpp"

std::string get_html_error(std::string const &status, std::string const &message);
std::string get_html_dir(std::string const &path, std::string const &root);

#endif
