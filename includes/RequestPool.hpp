#ifndef REQUEST_POOL_H
#define REQUEST_POOL_H

#include "HTTPStream.hpp"

class RequestPool
{
public:
	RequestPool(void);
	RequestPool(RequestPool const &ConnectionPool);
	~RequestPool(void);

	HTTPStream &getRequest(int fd);
	void		delRequest(int fd);

private:
	void growPool(void);
	void decrPool(void);

	size_t		_pool_size;
	HTTPStream *_request_pool;
	int		   *_fds;
	long		_active_connections;
};

#endif
