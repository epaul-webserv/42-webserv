#ifndef DATE_HPP
#define DATE_HPP

#include <ctime>
#include <exception>
#include <string>

typedef enum
{
    _rfc822,
    _rfc1123,
    _rfc850,
    _ansi,
    _invalid
} t_date_format;

class Date
{
  public:
    Date();
    Date(Date const &Date);
    Date(std::istream &stream_date);
    Date(std::string str_date);
    Date(time_t const *time);
    ~Date();

    Date          &operator=(Date const &Date);
    Date          &operator=(std::istream &str_date);
    Date          &operator=(std::string str_date);
    struct std::tm getDatetime() const;
    time_t         timestamp() const;
    bool           isPast() const;
    bool           operator<(Date const &rhs) const;
    bool           operator<=(Date const &rhs) const;
    bool           operator>(Date const &rhs) const;
    bool           operator>=(Date const &rhs) const;
    std::string    str() const;
    std::string    hstr() const;

    class InvalidFormatException : public std::exception
    {
      public:
        virtual const char *what() const throw();
    };

  private:
    bool          setDate(std::string &str_date);
    t_date_format try_get_format(const std::string &stream_date);
    bool          setRFC822(std::string &str_date);
    bool          setRFC1123(std::string &str_date);
    bool          setRFC850(std::string &str_date);
    bool          setANSI(std::string &str_date);

    struct std::tm _datetime;
};

std::ostream &operator<<(std::ostream &o, const Date &Date);

#endif
