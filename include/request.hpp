#ifndef GUARD_2CD7027DF48848B997879176FDE18E8E
#define GUARD_2CD7027DF48848B997879176FDE18E8E

#include <map>
#include <string>

#include "common_header.hpp"

class RequestData {
    /// NOTE: An empty string reflects a non-present header
    std::string common_headers[CommonHeader::MAX_SIZE];
    std::map<std::string, std::string> other_headers;
};

#endif