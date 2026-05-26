#ifndef CGI_HPP
# define CGI_HPP

# include <string>

class CGI
{
public:
    CGI(std::string executable_path) : executable_path(executable_path) {}
    CGI(CGI const &CGI) :
        executable_path(CGI.executable_path)
    {}
    ~CGI(void) {}

    bool    done();
    
private:
    CGI(void);
    std::string executable_path;
};

#endif
