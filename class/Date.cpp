#include "Date.hpp"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>

static std::string istostr(std::istream &is)
{
    return std::string(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>());
}

Date::Date()
{
    time_t     t    = time(0);
    struct tm *tm   = gmtime(&t);
    this->_datetime = *tm;
}

Date::Date(Date const &Date) : _datetime(Date._datetime) {}

Date::Date(std::string str_date)
{
    this->_datetime.tm_isdst = 1;
    if (!setDate(str_date))
        throw(Date::InvalidFormatException());
}

Date::Date(std::istream &stream_date)
{
    std::string str = istostr(stream_date);

    if (!setDate(str))
        throw(Date::InvalidFormatException());
}

Date::Date(time_t const *time) : _datetime(*gmtime(time)) {}

Date::~Date() {}

Date &Date::operator=(Date const &Date)
{
    this->_datetime = Date._datetime;
    return (*this);
}

Date &Date::operator=(std::istream &stream_date)
{
    std::string str = istostr(stream_date);

    setDate(str);
    return (*this);
}

Date &Date::operator=(std::string str_date)
{
    setDate(str_date);
    return (*this);
}

bool Date::setDate(std::string &str_date)
{
    t_date_format format = this->try_get_format(str_date);

    switch (format)
    {
    case _rfc822:
        return (this->setRFC822(str_date));
    case _rfc1123:
        return (this->setRFC1123(str_date));
    case _rfc850:
        return (this->setRFC850(str_date));
    case _ansi:
        return (this->setANSI(str_date));
    default:
        return (false);
    }
}

t_date_format Date::try_get_format(const std::string &str_date)
{
    size_t len = str_date.size();

    if (len > 4 && str_date[3] == ',')
    {
        if (len >= 27 && len <= 34)
        {
            if (str_date[14] == ' ')
                return (_rfc822);
            if (std::isdigit(static_cast<unsigned char>(str_date[14])))
                return (_rfc1123);
        }
    }
    else if (len > 4 && std::isalpha(static_cast<unsigned char>(str_date[3])))
    {
        if (len >= 30)
            return (_rfc850);
    }
    else if (len > 4 && str_date[3] == ' ')
    {
        if (len == 24)
            return (_ansi);
    }
    return (_invalid);
}

bool Date::setRFC822(std::string &str_date)
{
    char *rest = strptime(str_date.c_str(), "%a, %d %b %y %H:%M:%S", &this->_datetime);
    if (!rest)
        return (false);
    str_date = rest;
    if (str_date != " GMT")
        return (false);
    return (true);
}

bool Date::setRFC1123(std::string &str_date)
{
    char *rest = strptime(str_date.c_str(), "%a, %d %b %Y %H:%M:%S", &this->_datetime);
    if (!rest)
        return (false);
    str_date = rest;
    if (str_date != " GMT")
        return (false);
    return (true);
}

bool Date::setRFC850(std::string &str_date)
{
    char *rest = strptime(str_date.c_str(), "%A, %d-%b-%y %H:%M:%S", &this->_datetime);
    if (!rest)
        return (false);
    str_date = rest;
    if (str_date != " GMT")
        return (false);
    return (true);
}

bool Date::setANSI(std::string &str_date)
{
    char *rest = strptime(str_date.c_str(), "%a %b %d %H:%M:%S %Y", &this->_datetime);
    if (!rest)
        return (false);
    return (true);
}

const char *Date::InvalidFormatException::what() const throw()
{
    return ("Date format is invalid");
}

struct std::tm Date::getDatetime() const { return (this->_datetime); }

time_t Date::timestamp() const
{
    struct tm cpy = this->_datetime;

    return (timegm(&cpy));
}

bool Date::isPast() const
{
    struct tm cpy = this->_datetime;

    return (timegm(&cpy) < time(0));
}

bool Date::operator<(Date const &rhs) const { return (this->timestamp() < rhs.timestamp()); }

bool Date::operator<=(Date const &rhs) const { return (this->timestamp() <= rhs.timestamp()); }

bool Date::operator>(Date const &rhs) const { return (this->timestamp() > rhs.timestamp()); }

bool Date::operator>=(Date const &rhs) const { return (this->timestamp() >= rhs.timestamp()); }

std::string Date::str() const
{
    char buffer[80];

    std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", &this->_datetime);
    return (buffer);
}

std::string Date::hstr() const
{
    char buffer[80];

    std::strftime(buffer, sizeof(buffer), "%b %d %Y %H:%M", &this->_datetime);
    return (buffer);
}

std::ostream &operator<<(std::ostream &o, const Date &Date)
{
    o << Date.str();
    return (o);
}
