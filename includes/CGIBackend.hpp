#pragma once

#include <async>
#include <vector>
#include "ABackend.hpp"

class CGIBackend : public ABackend
{
public:
	CGIBackend(void);
	CGIBackend(std::vector<std::string> const &args);
	~CGIBackend(void);

	void generateResponse(Request const &request, Response &response, async::Reactor &reactor, async::BufIo<> &io);


private:
	void					 initResponse(Request const &request, Response &response, async::Reactor &reactor, async::BufIo<> &io);
	std::vector<std::string> _args;
};
