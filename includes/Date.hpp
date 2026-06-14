#ifndef DATE_HPP
#define DATE_HPP

#include <ctime>
#include <exception>
#include <string>

typedef enum
{
	RFC822,
	RFC1123,
	RFC850,
	ANSI,
	_invalid
} t_date_format;

class Date
{
public:
	Date();
	Date(Date const &Date);
	Date(std::istream &stream_date);
	Date(std::string const &str_date);
	Date(time_t const *time);
	~Date();

	Date		  &operator=(Date const &Date);
	Date		  &operator=(std::istream &stream_date);
	Date		  &operator=(std::string const &str_date);
	struct std::tm getDatetime() const;
	time_t		   timestamp() const;
	bool		   isPast() const;
	bool		   operator<(Date const &rhs) const;
	bool		   operator<=(Date const &rhs) const;
	bool		   operator>(Date const &rhs) const;
	bool		   operator>=(Date const &rhs) const;
	std::string	   str() const;
	std::string	   hstr() const;

	class InvalidFormatException : public std::exception
	{
	public:
		virtual const char *what() const throw();
	};

private:
	bool				 setDate(std::string const &str_date);
	static t_date_format try_get_format(std::string stream_date);
	bool				 setRFC822(std::istream &is_date);
	bool				 setRFC1123(std::istream &is_date);
	bool				 setRFC850(std::istream &is_date);
	bool				 setANSI(std::istream &is_date);

	struct std::tm _datetime;
};

std::ostream &operator<<(std::ostream &o, Date &Date);

#endif
