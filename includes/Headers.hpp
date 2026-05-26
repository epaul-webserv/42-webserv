#ifndef HEADERS_HPP
#define HEADERS_HPP

#include <map>
#include <string>

class Headers {
  public:
    Headers(void);
    Headers(Headers const &Headers);
    ~Headers(void);

    Headers const &operator=(Headers const &Headers);
    void clear(void);

  private:
    std::string _host;
    std::string _connection;
    std::string _user_agent;
    std::string _accept;
    std::string _cookie;
    std::map<std::string, std::string> _else;
};

#endif
