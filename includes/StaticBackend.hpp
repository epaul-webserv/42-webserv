#ifndef STATIC_RESPONSE_HPP
#define STATIC_RESPONSE_HPP

#include "Backend.hpp"
#include <string>
#include <sys/stat.h>

class StaticBackend : public Backend {
public:
    StaticBackend() : Backend() {
    }

    ~StaticBackend(void) {
    }

    void generateResponse(Request &request, Response &response);

private:
    void setHeaders();
    void sendHead(void);
    bool sendBody(void);
    void createErrorResponse(std::string const &statusCode,
                             std::string const &errorMessage);

    struct stat _sb;
};

#endif
