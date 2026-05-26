#ifndef BACKEND_HPP
#define BACKEND_HPP

#include "Request.hpp"
#include "Response.hpp"

class Backend {
  public:
    Backend(void);
    virtual ~Backend(void);

    virtual void generateResponse(Request const &request,
                                  Response &response) = 0;
};

#endif
