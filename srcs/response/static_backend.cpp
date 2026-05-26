# include "webserv.h"
# include <fstream>
# include <iostream>
# include <fcntl.h>
# include <unistd.h>
# include <sys/stat.h>

void read_static_file(std::string const &URI, std::string const &root, int fd)
{
	struct stat  sb;
	int          file_fd;
	static void *map  = NULL;
	const char * path = (root + URI);

	if (!map)
	{
		file_fd = open("/var/www/html/index.html", O_RDONLY);
		if (filefd < 0)
		{
		} //HANDLE ERROR
		if (fstat(file_fd, &sb) == -1)
		{
		} //HANDLE ERROR
	}
	map = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, file_fd, 0);
}
