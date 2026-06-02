#include "RequestPool.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>

RequestPool::RequestPool(void) : _pool_size(10), _request_pool(new Request[10]), _fds(new int[_pool_size]), _active_connections(0)
{
	std::fill(this->_fds, this->_fds + this->_pool_size, -1);
}

RequestPool::RequestPool(RequestPool const &ConnectionPool) : _pool_size(ConnectionPool._pool_size), _request_pool(new Request[this->_pool_size])
{
	for (size_t i; i < this->_pool_size; ++i)
		this->_request_pool[i] = ConnectionPool._request_pool[i];
}

RequestPool::~RequestPool(void)
{
	delete[] this->_request_pool;
	delete[] this->_fds;
}

Request &RequestPool::getRequest(int fd)
{
	// std::cout << "[DEBUG] Pool size : " << this->_pool_size << "\n[DEBUG]
	// Active connections : " << this->_active_connections << std::endl;
	for (size_t i = 0; i < _pool_size; ++i)
	{
		if (fd == _fds[i])
			return (this->_request_pool[i]);
	}
	for (size_t i = 0; i < _pool_size; ++i)
	{
		if (_fds[i] == -1)
		{
			this->_fds[i] = fd;
			this->_request_pool[i].clear(fd);
			++this->_active_connections;
			// std::cout << "[DEBUG] Pool size : " << this->_pool_size <<
			// "\n[DEBUG] Active connections : " << this->_active_connections <<
			// std::endl;
			return (this->_request_pool[i]);
		}
	}
	int reminder = this->_pool_size;
	growPool();
	++this->_active_connections;
	return (this->_request_pool[reminder]);
}

void RequestPool::delRequest(int fd)
{
	for (size_t i = 0; i < this->_pool_size; ++i)
	{
		if (fd == this->_fds[i])
		{
			this->_fds[i] = -1;
			--this->_active_connections;
			break;
		}
	}
	// if (this->_active_connections > 10 && this->_active_connections <
	// this->_pool_size / 10 * 0.8) 	decrPool();
}

void RequestPool::growPool(void)
{
	Request *temp_pool = new Request[this->_pool_size * 2];
	int		*temp_fds  = new int[this->_pool_size * 2];

	std::fill(temp_fds, (temp_fds + this->_pool_size * 2), -1);
	for (size_t i = 0; i < this->_pool_size; ++i)
		temp_pool[i] = this->_request_pool[i];
	std::memcpy(temp_fds, this->_fds, sizeof(int) * this->_pool_size);
	this->_pool_size *= 2;
	delete[] this->_request_pool;
	delete[] this->_fds;
	this->_request_pool = temp_pool;
	this->_fds			= temp_fds;
}

void RequestPool::decrPool(void)
{
	Request *temp_pool = new Request[this->_pool_size / 2];
	int		*temp_fds  = new int[this->_pool_size / 2];
	size_t	 j		   = 0;

	for (size_t i = 0; i < this->_pool_size; ++i)
	{
		if (this->_fds[i] != -1)
		{
			temp_fds[j]	 = this->_fds[i];
			temp_pool[j] = this->_request_pool[i];
			++j;
		}
	}
	this->_pool_size /= 2;
	while (++j < this->_pool_size)
		temp_fds[j] = -1;
	delete[] this->_request_pool;
	delete[] this->_fds;
	this->_request_pool = temp_pool;
	this->_fds			= temp_fds;
}
