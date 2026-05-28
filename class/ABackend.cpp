#include "ABackend.hpp"
#include "html_generator.hpp"

ABackend::ABackend(void)
{
}

ABackend::~ABackend(void)
{
}

void ABackend::setError(std::string const &status, std::string const &message,
						Response &         response)
{
	response.setHTTPStatus(status, message);
	response.bodyAppend(get_html_error(status, message));
}
