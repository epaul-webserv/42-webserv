#ifndef CGI_HPP
#define CGI_HPP

#include <string>
#include "ABackend.hpp"

class CGIBackend : public ABackend
{
public:
	CGIBackend(std::string executable_path) : _executable_path(executable_path)
	{
	}

	~CGIBackend(void) {}

	void generateResponse(Request const &request, Response &response);

private:
	CGIBackend(void);
	std::string _executable_path;
};

#endif
