#ifndef MIME_TYPE_HPP
#define MIME_TYPE_HPP

#include <map>
#include <string>

class MimeType
{

  public:
    static std::map<std::string, std::string> mime_type;
    static void        setType(std::string const &key, std::string const &value);
    static std::string getType(std::string const &URI);
    static bool                               IsValidExtension(const std::string &ext);
    static void                               ParseConfig(const char *file);

  private:
    MimeType();
    MimeType(const MimeType &other);
    MimeType &operator=(MimeType const &other);
    ~MimeType();
};

#endif
