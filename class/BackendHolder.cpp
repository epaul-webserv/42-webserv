#include "BackendHolder.hpp"
#include "MimeType.hpp"

BackendHolder::BackendHolder(t_backend_type backend_type, std::vector<std::string> const &cgi_args) : backend_type(backend_type), cgi_args(cgi_args)
{
	MimeType::mime_type.insert(std::pair<std::string, std::string>("svg", "image/svg+xml"));
	MimeType::mime_type.insert(std::pair<std::string, std::string>("png", "image/png"));
	MimeType::mime_type.insert(std::pair<std::string, std::string>("html", "text/html"));
	MimeType::mime_type.insert(std::pair<std::string, std::string>("mkv", "video/x-matroska"));
	if (backend_type == _cgi)
		this->addr = (new CGIBackend(cgi_args));
	else
		this->addr = (new StaticBackend());
}

BackendHolder::BackendHolder(BackendHolder const &BackendHolder) : backend_type(BackendHolder.backend_type), cgi_args(BackendHolder.cgi_args)
{
	MimeType::mime_type.insert(std::pair<std::string, std::string>("svg", "image/svg+xml"));
	MimeType::mime_type.insert(std::pair<std::string, std::string>("png", "image/png"));
	MimeType::mime_type.insert(std::pair<std::string, std::string>("html", "text/html"));
	MimeType::mime_type.insert(std::pair<std::string, std::string>("mkv", "video/x-matroska"));
	if (backend_type == _cgi)
		this->addr = (new CGIBackend(cgi_args));
	else
		this->addr = (new StaticBackend());
}

BackendHolder::~BackendHolder(void) { delete this->addr; }
