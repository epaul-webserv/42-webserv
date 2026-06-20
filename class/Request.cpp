#include "Request.hpp"
#include "string_overload.hpp"
#include "webserv.hpp"

Request::Request() {}

Request::Request(Request const &Request)
    : _method(Request._method), _URI(Request._URI), _http_version(Request._http_version),
      _headers(Request._headers)
{
}

Request::~Request() {}

Request &Request::operator=(Request const &other)
{
    if (this != &other)
    {
        this->_method       = other._method;
        this->_URI          = other._URI;
        this->_http_version = other._http_version;
        this->_headers      = other._headers;
    }
    return (*this);
}

void Request::clear()
{
    this->_method.clear();
    this->_URI.clear();
    this->_http_version.clear();
    this->_headers.clear();
}

void Request::prepareNextRead()
{
    this->_method.clear();
    this->_URI.clear();
    this->_http_version.clear();
    this->_headers.clear();
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
        it = this->_headers.begin();
    else
    {
        std::string tmp = key;
        to_lower(tmp);
        it = this->_headers.find(tmp);
    }
    if (it == this->_headers.end())
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
        this->_headers.erase(key);
    }
    else if (this->_headers.begin() != this->_headers.end())
    {
        ret = *this->_headers.begin();
        this->_headers.erase(this->_headers.begin());
    }
    return (ret);
}

std::string Request::getMethod() const { return (this->_method); }

std::string Request::getURI() const { return (this->_URI); }

void Request::setURI(std::string const &URI) { this->_URI = URI; }

std::string Request::getQueryString() const { return (this->_query_string); }

std::string Request::getHTTPVersion() const { return (this->_http_version); }

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
