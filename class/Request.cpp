#include "Request.hpp"
#include "string_overload.hpp"
#include "webserv.hpp"

Request::Request() {}

Request::Request(Request const &Request)
    : method(Request.method), URI(Request.URI), original_URI(Request.original_URI),
      rewrote_URI(Request.rewrote_URI), query_string(Request.query_string),
      http_version(Request.http_version), headers(Request.headers)
{
}

Request::~Request() {}

Request &Request::operator=(Request const &other)
{
    if (this != &other)
    {
        this->method       = other.method;
        this->URI          = other.URI;
        this->original_URI = other.original_URI;
        this->rewrote_URI  = other.rewrote_URI;
        this->query_string = other.query_string;
        this->http_version = other.http_version;
        this->headers      = other.headers;
    }
    return (*this);
}

void Request::clear()
{
    this->method.clear();
    this->URI.clear();
    this->original_URI.clear();
    this->rewrote_URI.clear();
    this->query_string.clear();
    this->http_version.clear();
    this->headers.clear();
}

void Request::prepareNextRead()
{
    this->method.clear();
    this->URI.clear();
    this->original_URI.clear();
    this->rewrote_URI.clear();
    this->query_string.clear();
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

const char *Request::InvalidReadException::what() const throw() { return ("Error while reading"); }

std::ostream &operator<<(std::ostream &o, Request &request)
{
    o << "-- REQUEST --" << std::endl;
    o << "method : \"" << request.method << "\"" << std::endl;
    o << "URI : \"" << request.URI << "\"" << std::endl;
    o << "version : \"" << request.http_version << "\"" << std::endl;
    o << "-- HEADERS --" << std::endl;
    for (std::pair<std::string, std::string> header = request.takeHeader(); !header.first.empty();
         header                                     = request.takeHeader())
        o << "\"" << header.first << "\" : \"" << header.second << "\"" << std::endl;
    o << "-------------" << std::endl;
    return (o);
}
