#pragma once

#include "ABackend.hpp"

class CGIBackend : public ABackend
{
public:
	CGIBackend(void);
	CGIBackend(std::string const &path);
	~CGIBackend(void);

	void generateResponse(Request const &request, Response &response);

private:
	std::string _path;
};
