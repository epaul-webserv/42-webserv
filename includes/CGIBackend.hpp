#pragma once

#include <vector>
#include "ABackend.hpp"

class CGIBackend : public ABackend
{
public:
	CGIBackend(void);
	CGIBackend(std::vector<std::string> const &args);
	~CGIBackend(void);

	void generateResponse(Request const &request, Response &response);

private:
	std::vector<std::string> _args;
};
