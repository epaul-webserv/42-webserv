#ifndef REQUEST_POOL_H
#define REQUEST_POOL_H

#include "Request.hpp"

class RequestPool {
  public:
    RequestPool(void);
    RequestPool(RequestPool const &ConnectionPool);
    ~RequestPool(void);

    Request &getRequest(int fd);
    void delRequest(int fd);

  private:
    void growPool(void);
    void decrPool(void);

    size_t _pool_size;
    Request *_request_pool;
    int *_fds;
    long _active_connections;
};

#endif
