#ifndef IREQUESTBODY_HPP
#define IREQUESTBODY_HPP

#include <async>
#include <exception>

class IRequestBody
{
  public:
    virtual async::Future::Poll poll_read(container::Slice<uint8_t> &buf, size_t &s) = 0;
    virtual ~IRequestBody();

    class TooLargeException : public std::exception
    {
      public:
        const char *what() const throw();
    };

    class BadReadException : public std::exception
    {
      public:
        const char *what() const throw();
    };
};

#endif
