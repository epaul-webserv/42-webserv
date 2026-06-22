#include "Request.hpp"
#include "string_overload.hpp"
#include "webserv.hpp"

Request::Request() {}

Request::Request(Request const &Request)
    : method(Request.method), URI(Request.URI), http_version(Request.http_version),
      headers(Request.headers)
{
}

Request::~Request() {}

Request &Request::operator=(Request const &other)
{
    if (this != &other)
    {
        this->method       = other.method;
        this->URI          = other.URI;
        this->http_version = other.http_version;
        this->headers      = other.headers;
    }
    return (*this);
}

void Request::clear()
{
    this->method.clear();
    this->URI.clear();
    this->http_version.clear();
    this->headers.clear();
}

void Request::prepareNextRead()
{
    this->method.clear();
    this->URI.clear();
    this->http_version.clear();
    this->headers.clear();
}

/// @brief returns the next header available.
/// This function does not delete the returned header
/// (use getHeader for deletion).
/// @param key Key of the requested header, do not send to get the next header
/// @return value of the requested header or empty string if it does not exist
std::string Request::peekHeader(std::string const &key) const
{
    std::map<std::string, std::string>::const_iterator it;

    if (key.empty())
        it = this->headers.begin();
    else
    {
        std::string tmp = key;
        to_lower(tmp);
        it = this->headers.find(tmp);
    }
    if (it == this->headers.end())
        return ("");
    return (it->second);
}

/// @brief returns the next header available.
/// This function deletes the returned header
/// (use peekHeader if this behavior is not desired).
/// @param key Key of the requested header, do not send to get the next header
/// @return value of the requested header or empty string if it does not exist
std::pair<std::string, std::string> Request::takeHeader(std::string const &key)
{
    std::pair<std::string, std::string> ret;

    if (!key.empty())
    {
        ret = std::pair<std::string, std::string>(key, peekHeader(key));
        this->headers.erase(key);
    }
    else if (this->headers.begin() != this->headers.end())
    {
        ret = *this->headers.begin();
        this->headers.erase(this->headers.begin());
    }
    return (ret);
}

std::string Request::getMethod() const { return (this->method); }

std::string Request::getURI() const { return (this->URI); }

void Request::setURI(std::string const &URI) { this->URI = URI; }

std::string Request::getQueryString() const { return (this->query_string); }

std::string Request::getHTTPVersion() const { return (this->http_version); }

const char *Request::InvalidReadException::what() const throw() { return ("Error while reading"); }

std::ostream &operator<<(std::ostream &o, Request &request)
{
    o << "-- REQUEST --" << std::endl;
    o << "method : \"" << request.getMethod() << "\"" << std::endl;
    o << "URI : \"" << request.getURI() << "\"" << std::endl;
    o << "version : \"" << request.getHTTPVersion() << "\"" << std::endl;
    o << "-- HEADERS --" << std::endl;
    for (std::pair<std::string, std::string> header = request.takeHeader(); !header.first.empty();
         header                                     = request.takeHeader())
        o << "\"" << header.first << "\" : \"" << header.second << "\"" << std::endl;
    o << "-------------" << std::endl;
    return (o);
}
