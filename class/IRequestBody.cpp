#include "IRequestBody.hpp"

IRequestBody::~IRequestBody() {}

const char *IRequestBody::TooLargeException::what() const throw() { return ("Body Too Large"); }

const char *IRequestBody::BadReadException::what() const throw()
{
    return ("Error while reading body");
}
