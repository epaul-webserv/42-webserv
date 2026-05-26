#ifndef RESPONSE_POOL_HPP
#define RESPONSE_POOL_HPP

#include "Response.hpp"

class ResponsePool {
    ResponsePool(void) {}

    ~ResponsePool(void) {}

    Response &getResponse(int fd);
    void delResponse(int fd);

  private:
    ResponsePool(ResponsePool const &ConnectionPool);

    std::map<int, Response *> _response_pool;
};

#endif
