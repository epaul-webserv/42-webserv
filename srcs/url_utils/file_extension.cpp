#include <string>

///@brief Extracts the complete filename from a correct URL
std::string get_file_name(std::string const &URI)
{
    size_t beg = URI.find_last_of('/');
    size_t end = URI.find_first_of('?');

    return (URI.substr((beg == std::string::npos) ? 0 : beg, end));
}
