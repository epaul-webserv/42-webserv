#include "ABackend.hpp"

ABackend::ABackend(void)
{
}

ABackend::~ABackend(void)
{
}

void ABackend::setError(std::string const &status, std::string const &message)
{
	this->headAppend(
		this->_http_version + " " + status + " " + message + "\r\n");
	this->bodyAppend(get_html_error(status, message));
}
