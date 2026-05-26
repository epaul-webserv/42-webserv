#ifndef DATE_HPP
#define DATE_HPP

#include <ctime>
#include <exception>
#include <string>

typedef enum { _RFC822, _RFC1123, _RFC850, _ANSI, _invalid } t_date_format;

class Date {
  public:
    Date(void);
    Date(Date const &Date);
    Date(std::istream &stream_date);
    Date(std::string const &str_date);
    Date(time_t const *time);
    ~Date(void);

    Date const &operator=(Date const &Date);
    Date const &operator=(std::istream &stream_date);
    Date const &operator=(std::string const &str_date);
    struct std::tm getDatetime(void) const;
    time_t timestamp(void) const;
    bool isPast(void) const;
    bool operator<(Date const &rhs);
    bool operator<=(Date const &rhs);
    bool operator>(Date const &rhs);
    bool operator>=(Date const &rhs);
    std::string str(void) const;
    std::string hstr(void) const;

    class InvalidFormatException : public std::exception {
      public:
        virtual const char *what() const throw();
    };

  private:
    bool setDate(std::string const &str_date);
    t_date_format try_get_format(std::string stream_date);
    bool setRFC822(std::istream &str_date);
    bool setRFC1123(std::istream &str_date);
    bool setRFC850(std::istream &str_date);
    bool setANSI(std::istream &str_date);

    struct std::tm _datetime;
};

std::ostream &operator<<(std::ostream &o, Date &Date);

#endif
