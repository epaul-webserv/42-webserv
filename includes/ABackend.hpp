#ifndef ABACKEND_HPP
#define ABACKEND_HPP

#include <async>

typedef enum
{
    _static,
    _cgi,
    _dir_index,
    _upload
} t_backend_type;

class ABackend : public async::Future
{
  public:
    virtual ~ABackend();
    virtual Poll poll() = 0;
};

#endif
