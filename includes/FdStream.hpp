#ifndef FDSTREAM_HPP
#define FDSTREAM_HPP

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <string>
#include <unistd.h>
#include <vector>
#include "stdint.h"

#define FDSTREAM_BUFF_SIZE 8192

class FdStream
{
public:
	FdStream(int fd);
	~FdStream(void);


	FdStream			&operator=(FdStream const &FdStream);
	bool				 end(void) const;
	std::string			 get_str(size_t n);
	void				 consume(size_t n);
	std::vector<uint8_t> peek_str(size_t n);
	std::string			 get_str_until(char c);
	bool				 is_open(void) const;
	void				 clear(int fd);

private:
	FdStream(FdStream const &FdStream) { (void)FdStream; };
	void fillBuffer(void);

	bool   _end;
	int	   _fd;
	char   _buffer[FDSTREAM_BUFF_SIZE];
	size_t _read_index;
	size_t _write_index;

	class InvalidFdException : public std::exception
	{
		const char *what(void) const throw();
	};

	class InvalidReadException : public std::exception
	{
		const char *what(void) const throw();
	};
};

#endif
