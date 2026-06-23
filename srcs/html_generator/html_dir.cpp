#include "../includes/html_generator.hpp"
#include "url_utils.hpp"

static std::string get_filetype_str(unsigned char type)
{
    switch (type)
    {
    case DT_DIR:
        return ("dir");
    default:
        return ("file");
    }
}

std::string get_filled_line(const std::string &sample_line, struct dirent *file,
                            std::string const &path, std::string const &root)
{
    const char       *c_sample_line = sample_line.c_str();
    size_t            ebpos         = 0;
    struct stat       filestat;
    std::stringstream o;

    if (stat((root + path + (path == "/" ? "" : "/") + file->d_name).c_str(), &filestat))
        return (o.str());
    for (size_t bpos = sample_line.find('[', 0); bpos != std::string::npos;
         bpos        = sample_line.find('[', ebpos))
    {
        o << sample_line.substr(ebpos, bpos - ebpos);
        ebpos = sample_line.find(']', bpos) + 1;
        if (!std::strncmp(c_sample_line + bpos, "[FILETYPE]", 10))
            o << get_filetype_str(file->d_type);
        else if (!std::strncmp(c_sample_line + bpos, "[FILEPATH]", 10))
            o << path << "/" << encode_url(file->d_name) << (file->d_type == DT_DIR ? "/" : "");
        else if (!std::strncmp(c_sample_line + bpos, "[FILENAME]", 10))
            o << file->d_name << (file->d_type == DT_DIR ? "/" : "")
              << std::setw(50 - std::strlen(file->d_name)) << std::left << "";
        else if (!std::strncmp(c_sample_line + bpos, "[FILELINK]", 10))
            o << "<a href=\"" << encode_url(file->d_name) << (file->d_type == DT_DIR ? "/" : "")
              << "\" class=\"" << get_filetype_str(file->d_type) << "\">" << file->d_name
              << (file->d_type == DT_DIR ? "/" : "") << "</a>"
              << std::setw(50 - (file->d_type == DT_DIR ? 1 : 0) - std::strlen(file->d_name))
              << std::left << "";
        else if (!std::strncmp(c_sample_line + bpos, "[FILEDATE]", 10))
            o << Date(&filestat.st_mtime).hstr();
        else if (!std::strncmp(c_sample_line + bpos, "[FILESIZE]", 10))
        {
            if (file->d_type == DT_DIR)
                o << std::setw(20) << std::right << "-";
            else
            {
                std::stringstream size;
                size << filestat.st_size;
                o << std::setw(20) << std::right << size.str();
            }
        }
    }
    if (ebpos < sample_line.size())
        o << sample_line.substr(ebpos, sample_line.size() - ebpos);
    return (o.str());
}

static std::string list_files(std::string const &sample_line, DIR *dir, std::string const &path,
                              std::string const &root)
{
    std::map<std::string, struct dirent *> files;
    std::string                            dest;

    for (struct dirent *file = readdir(dir); file; file = readdir(dir))
    {
        if (std::strcmp(file->d_name, "."))
            files.insert(std::pair<std::string, struct dirent *>(file->d_name, file));
    }
    for (std::map<std::string, struct dirent *>::iterator it = files.begin(), ite = files.end();
         it != ite; ++it)
    {
        if (it->second->d_type == DT_DIR && (it->first == ".." || it->first[0] != '.'))
            dest.append(get_filled_line(sample_line, it->second, path, root) + '\n');
    }
    for (std::map<std::string, struct dirent *>::iterator it = files.begin(), ite = files.end();
         it != ite; ++it)
    {
        if (it->second->d_type != DT_DIR && it->first[0] != '.')
            dest.append(get_filled_line(sample_line, it->second, path, root) + '\n');
    }
    return (dest);
}

std::string fill_blanks(DIR *dir, FdStream &fdstream, std::string const &path,
                        std::string const &root)
{
    std::string html;
    std::string temp;

    while (!fdstream.eof())
    {
        html.append(fdstream.get_str_until('['));
        if (fdstream.eof())
            break;
        temp = fdstream.get_str_until(']');
        if (temp == "PATH")
            html.append(path);
        else if (temp == "FILELINE")
            html.append(list_files(fdstream.get_str_until('\n'), dir, path, root));
    }
    return (html);
}

std::string get_html_dir(std::string const &path, std::string const &root)
{
    DIR *dir;
    int  fd = open("srcs/html_generator/dir_sample.html", O_RDONLY);
    if (fd == -1)
        throw(std::runtime_error("Template is absent"));
    FdStream    fdstream(fd);
    std::string html;

    dir = opendir((root + path).c_str());
    if (!dir)
        throw(std::runtime_error("Error while opening directory"));
    html = fill_blanks(dir, fdstream, path, root);
    closedir(dir);
    close(fd);
    return (html);
}
