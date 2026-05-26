#include "webserv.h"

int init_listener(struct sockaddr_in &addr, socklen_t &addrlen)
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	int opt    = 1;
	if (sockfd < 0)
		return (-1);
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		close(sockfd);
		return (-1);
	}
	if (bind(sockfd, (sockaddr *)&addr, addrlen) < 0)
	{
		close(sockfd);
		return (-1);
	}
	if (listen(sockfd, 1000) < 0)
	{
		close(sockfd);
		return (-1);
	}
	return (sockfd);
}
