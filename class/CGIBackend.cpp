#include "CGIBackend.hpp"

void CGIBackend::generateResponse(Request const &request, Response &response)
{
	(void)request;
	response.setHTTPVersion("HTTP/1.1");
	response.bodyAppend("this is the CGI backend with the path " +
						this->_executable_path);
	response.setStatus(_ready);
}
