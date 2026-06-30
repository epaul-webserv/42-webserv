#include "html_generator.hpp"

std::string get_html_error(std::string const &status, std::string const &message)
{
    int fd = open("srcs/html_generator/error_sample.html", O_RDONLY);
    if (fd < 0)
        return ("Error " + status + " : " + message);
    FdStream    fdstream(fd);
    std::string temp;
    std::string html;

    if (status == "[STATUS]" || message == "[MESSAGE]")
        return ("Error " + status + " : " + message);
    while (!fdstream.eof())
    {
        html.append(fdstream.get_str_until('['));
        if (fdstream.eof())
            break;
        temp = fdstream.get_str_until(']');
        if (temp == "STATUS")
            html.append(status);
        if (temp == "MESSAGE")
            html.append(message);
    }
    return (html);
}
