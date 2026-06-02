#include "Date.hpp"
#include <cctype>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

static std::string istostr(std::istream &is) { return std::string(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>()); }

Date::Date(void)
{
	time_t t		= time(0);
	this->_datetime = *gmtime(&t);
}

Date::Date(Date const &Date) : _datetime(Date._datetime) {}

Date::Date(std::string const &str_date)
{
	this->_datetime.tm_isdst = 1;
	if (!setDate(str_date))
		throw(Date::InvalidFormatException());
}

Date::Date(std::istream &stream_date)
{
	if (!setDate(istostr(stream_date)))
		throw(Date::InvalidFormatException());
}

Date::Date(time_t const *time) : _datetime(*gmtime(time)) {}

Date::~Date(void) {}

Date const &Date::operator=(Date const &Date)
{
	this->_datetime = Date._datetime;
	return (*this);
}

Date const &Date::operator=(std::istream &stream_date)
{
	setDate(istostr(stream_date));
	return (*this);
}

Date const &Date::operator=(std::string const &str_date)
{
	setDate(str_date);
	return (*this);
}

bool Date::setDate(std::string const &str_date)
{
	t_date_format	   format = this->try_get_format(str_date);
	std::istringstream iss_date(str_date);

	switch (format)
	{
	case _RFC822:
		return (this->setRFC822(iss_date));
	case _RFC1123:
		return (this->setRFC1123(iss_date));
	case _RFC850:
		return (this->setRFC850(iss_date));
	case _ANSI:
		return (this->setANSI(iss_date));
	default:
		return (false);
	}
}

t_date_format Date::try_get_format(std::string str_date)
{
	size_t len = str_date.size();

	if (len > 4 && str_date[3] == ',')
	{
		if (len >= 27 && len <= 34)
		{
			if (str_date[14] == ' ')
				return (_RFC822);
			if (std::isdigit(str_date[14]))
				return (_RFC1123);
		}
	}
	else if (len > 4 && std::isalpha(str_date[3]))
	{
		if (len >= 30)
			return (_RFC850);
	}
	else if (len > 4 && str_date[3] == ' ')
	{
		if (len == 24)
			return (_ANSI);
	}
	return (_invalid);
}

bool Date::setRFC822(std::istream &is_date)
{
	char						  *rest;
	std::istreambuf_iterator<char> start(is_date), end;
	std::string					   str(start, end);

	rest = strptime(str.c_str(), "%a, %d %b %y %H:%M:%S", &this->_datetime);
	if (!rest)
		return (false);
	str = rest;
	if (str != " GMT")
		return (false);
	return (true);
}

bool Date::setRFC1123(std::istream &is_date)
{
	char						  *rest;
	std::istreambuf_iterator<char> start(is_date), end;
	std::string					   str(start, end);

	rest = strptime(str.c_str(), "%a, %d %b %Y %H:%M:%S", &this->_datetime);
	if (!rest)
		return (false);
	str = rest;
	if (str != " GMT")
		return (false);
	return (true);
}

bool Date::setRFC850(std::istream &is_date)
{
	char						  *rest;
	std::istreambuf_iterator<char> start(is_date), end;
	std::string					   str(start, end);

	rest = strptime(str.c_str(), "%A, %d-%b-%y %H:%M:%S", &this->_datetime);
	if (!rest)
		return (false);
	str = rest;
	if (str != " GMT")
		return (false);
	return (true);
}

bool Date::setANSI(std::istream &is_date)
{
	char						  *rest;
	std::istreambuf_iterator<char> start(is_date), end;
	std::string					   str(start, end);

	rest = strptime(str.c_str(), "%a %b %d %H:%M:%S %Y", &this->_datetime);
	if (!rest)
		return (false);
	return (true);
}

const char *Date::InvalidFormatException::what(void) const throw() { return ("Date format is invalid"); }

struct std::tm Date::getDatetime(void) const { return (this->_datetime); }

time_t Date::timestamp(void) const
{
	struct tm cpy = this->_datetime;

	return (timegm(&cpy));
}

bool Date::isPast(void) const
{
	struct tm cpy = this->_datetime;

	return (timegm(&cpy) < time(0));
}

bool Date::operator<(Date const &rhs) { return (this->timestamp() < rhs.timestamp()); }

bool Date::operator<=(Date const &rhs) { return (this->timestamp() <= rhs.timestamp()); }

bool Date::operator>(Date const &rhs) { return (this->timestamp() > rhs.timestamp()); }

bool Date::operator>=(Date const &rhs) { return (this->timestamp() >= rhs.timestamp()); }

std::string Date::str(void) const
{
	char buffer[80];

	std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", &this->_datetime);
	return (buffer);
}

std::string Date::hstr(void) const
{
	char buffer[80];

	std::strftime(buffer, sizeof(buffer), "%b %d %Y %H:%M", &this->_datetime);
	return (buffer);
}

std::ostream &operator<<(std::ostream &o, Date &Date)
{
	o << Date.str();
	return (o);
}
