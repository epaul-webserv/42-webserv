#ifndef ABACKEND_HPP
#define ABACKEND_HPP

#include "Request.hpp"
#include "Response.hpp"

class ABackend
{
public:
	ABackend(void);
	virtual ~ABackend(void);

	virtual void generateResponse(Request const &request,
								  Response		&response) = 0;
	void		 setError(std::string const &status, std::string const &message,
						  Response &response);
};

#endif
