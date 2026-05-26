#ifndef FDSTREAM_HPP
# define FDSTREAM_HPP

# include <cstddef>
# include <unistd.h>
# include <string>
# include <algorithm>
# include <cerrno>
# include <cstring>

# define FDSTREAM_BUFF_SIZE 8192

class FdStream
{
public:
	FdStream(int fd);
	~FdStream(void);

	bool        end(void) const;
	std::string get_str(size_t n);
	std::string get_str_until(char c);
	void        clear(int fd);

private:
	FdStream(FdStream const &FdStream) { (void)FdStream; };

	FdStream const &operator=(FdStream const &FdStream)
	{
		(void)FdStream;
		return *this;
	};
	void fillBuffer(void);

	bool   _end;
	int    _fd;
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
