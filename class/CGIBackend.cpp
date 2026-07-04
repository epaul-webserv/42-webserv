#include "CGIBackend.hpp"
#include "url_utils.hpp"
#include "webserv.hpp"
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <vector>

CGIBackend::CGIBackend(std::vector<std::string> const &args, std::string const &root,
                       async::Reactor &reactor, HTTPStream &stream)
    : _root(root), _reactor(reactor), _args(args), _cgi_io(NULL), stream(stream),
      response_body(NULL), _cgi_pid(0), _body_read_buf_len(0), _body_read_buf_index(0),
      _client_body_sent(false), _response_headers_read_done(false), _status_set(false),
      _process_done(false), _content_length(-1), _status(_getting_request_body),
      client_body_reader(NULL), _response_buf_len(0), _response_buf_index(0),
      _response_body_sent(false), _send_header_and_abort(false)
{
    client_body_reader = stream.request_body();
}

CGIBackend::~CGIBackend()
{
    if (_cgi_io != NULL)
        delete _cgi_io;
    delete client_body_reader;
    delete response_body;
    if (_cgi_pid > 0)
    {
        kill(_cgi_pid, SIGKILL);
        waitpid(_cgi_pid, NULL, 0);
    }
}

async::Future::Poll CGIBackend::poll()
{
    if (_status == _getting_request_body)
    {
        if (get_body() == Pending)
            return Pending;
        _status = _init;
    }

    if (_status == _init)
        init();
    check_process_status();
    if (_send_header_and_abort)
    {
        if (!response_body && stream.poll_send_headers(response_body) == Pending)
            return Pending;
        if (response_body)
            return (response_body->poll_end());
        return Ready;
    }

    send_client_body_to_cgi();
    read_cgi_headers();
    if (_send_header_and_abort)
    {
        if (!response_body && stream.poll_send_headers(response_body) == Pending)
            return Pending;
        if (response_body)
            return (response_body->poll_end());
        return Ready;
    }

    if (set_body_writer() == Ready)
        return Ready;
    send_body();
    if (_response_body_sent)
    {
        if (response_body->poll_end() == Pending)
            return Pending;
        return Ready;
    }
    return Pending;
}

void CGIBackend::check_process_status()
{
    if (_cgi_pid > 0)
    {
        int status;
        int ret = waitpid(_cgi_pid, &status, WNOHANG);
        if (ret > 0)
        {
            _cgi_pid = -1;
            if (WIFEXITED(status))
            {
                if (WEXITSTATUS(status) != 0)
                {
                    if (!_response_headers_read_done)
                        stream.setErrorResponse("500", "Internal Server Error");
                    stream.setBrokenConnection();
                    _send_header_and_abort = true;
                    return;
                }
                else
                    _process_done = true;
            }
            else
            {
                stream.setBrokenConnection();
                _send_header_and_abort = true;
                return;
            }
        }
        else if (ret < 0) // this will probably never happen, idk
        {
            std::cerr << "waitpid error" << std::endl;
            if (!_response_headers_read_done)
                stream.setErrorResponse("500", "Internal Server Error");
            else
                stream.setBrokenConnection();
            _send_header_and_abort = true;
            return;
        }
    }
}

bool CGIBackend::parse_cgi_header(const std::vector<uint8_t> &raw_line, std::string &key,
                                  std::string &value)
{
    if (raw_line.empty())
        return false;

    std::string line(reinterpret_cast<const char *>(&raw_line[0]), raw_line.size());

    while (!line.empty() && (line.at(line.size() - 1) == '\n' || line.at(line.size() - 1) == '\r'))
        line.erase(line.size() - 1);

    if (line.empty())
        return false;

    size_t colon_pos = line.find(':');
    if (colon_pos == std::string::npos || colon_pos == 0)
        return false;

    size_t key_start = 0;
    size_t key_end   = colon_pos;

    while (key_end > key_start && (line[key_end - 1] == ' ' || line[key_end - 1] == '\t'))
        --key_end;

    if (key_start == key_end)
        return false;

    size_t val_start = colon_pos + 1;
    size_t val_end   = line.size();

    while (val_start < val_end && (line[val_start] == ' ' || line[val_start] == '\t'))
        ++val_start;
    while (val_end > val_start && (line[val_end - 1] == ' ' || line[val_end - 1] == '\t'))
        --val_end;
    key   = to_lower(line.substr(key_start, key_end - key_start));
    value = line.substr(val_start, val_end - val_start);

    return true;
}

void CGIBackend::parse_cgi_status_value(const std::string &status_value, std::string &status_code,
                                        std::string &status_message)
{
    status_code.clear();
    status_message.clear();

    std::string::size_type space_pos = status_value.find(' ');

    if (space_pos == std::string::npos)
        status_code = status_value;
    else
    {
        status_code                      = status_value.substr(0, space_pos);
        std::string::size_type msg_start = space_pos;
        while (msg_start < status_value.size() &&
               (status_value[msg_start] == ' ' || status_value[msg_start] == '\t'))
            ++msg_start;

        if (msg_start < status_value.size())
            status_message = status_value.substr(msg_start);
    }
}

bool CGIBackend::check_files_validity(const char *cgi_name)
{
    if (access(cgi_name, X_OK) != 0)
    {
        std::cerr << "Error: CGI program is not executable or does not exist.\n";
        stream.setErrorResponse("500", "Internal Server Error");
        _send_header_and_abort = true;
        return false;
    }
    return true;
}

std::string get_document_root(std::string file)
{
    size_t n = file.find_last_of('/');
    return file.substr(0, n);
}

async::Future::Poll CGIBackend::get_body()
{
    if (client_body_reader == NULL)
    {
        _status = _init;
        return Ready;
    }
    size_t                    s;
    container::Slice<uint8_t> buff(_body_read_buf, sizeof(_body_read_buf));
    while (client_body_reader->poll_read(buff, s) == Ready)
    {
        if (s == 0)
            return Ready;
        request_body.insert(request_body.end(), buff.begin(), buff.begin() + s);
    }
    return Pending;
}

#include "string_overload.hpp"
// The request has been routed towards the cgi, we start the program and setup
// the io to process it
void CGIBackend::init()
{
    _status                           = _processing;
    std::string              encoding = stream.request.peekHeader("transfer-encoding");
    std::vector<std::string> env_vector;

    env_vector.push_back("QUERY_STRING=" + stream.request.query_string);
    env_vector.push_back("REQUEST_METHOD=" + stream.request.method);

    // The name of the file being executed

    if (!check_files_validity(_args[0].c_str()))
        return;

    env_vector.push_back("PATH_TRANSLATED=" + std::string(_root) + "/" +
                         get_file_name(stream.request.URI));

    env_vector.push_back("SERVER_ADDR=" + toString(stream.server_host));
    env_vector.push_back("SERVER_PORT=" + toString(stream.server_port));

    // env_vector.push_back("SERVER_NAME=" + std::string(PROGNAME)); // REMOVE
    // name of the program
    env_vector.push_back("SERVER_SOFTWARE=" + std::string(PROGNAME));
    // protocol
    env_vector.push_back("SERVER_PROTOCOL=" + stream.request.http_version);

    // TESTER
    // env_vector.push_back("PATH_INFO=" + std::string(_root) + "/" +
    //                      get_file_name(stream.request.URI));
    // env_vector.push_back("SCRIPT_FILENAME=" + std::string(_root) + "/" +
    //                      get_file_name(stream.request.URI));

    // WORDPRESS
    env_vector.push_back("PATH_INFO=");
    env_vector.push_back("SCRIPT_FILENAME=" + _root + stream.request.rewrote_URI); // for
    env_vector.push_back("DOCUMENT_ROOT=" + _root);
    env_vector.push_back("SCRIPT_NAME=" + stream.request.rewrote_URI);
    std::string request_uri = stream.request.original_URI;
    if (request_uri.empty())
        request_uri = stream.request.URI;
    if (!stream.request.query_string.empty())
        request_uri += "?" + stream.request.query_string;
    env_vector.push_back("REQUEST_URI=" + request_uri);

    if (!request_body.empty())
    {
        env_vector.push_back("CONTENT_LENGTH=" + toString(request_body.size()));
        if (stream.request.peekHeader("content-type").empty())
        {
            stream.setErrorResponse("400", "Bad Request");
            _send_header_and_abort = true;
            return;
        }
        env_vector.push_back("CONTENT_TYPE=" + stream.request.peekHeader("content-type"));
    }

    for (std::map<std::string, std::string>::const_iterator it = stream.request.headers.begin();
         it != stream.request.headers.end(); ++it)
    {
        std::string header_name = it->first;
        std::string header_val  = it->second;
        if (header_name == "content-type" || header_name == "content-length")
            continue;
        // if (header_name == "authorization" || header_name == "connection")
        //     continue;
        for (size_t i = 0; i < header_name.length(); ++i)
        {
            if (header_name[i] == '-')
                header_name[i] = '_';
            else
                header_name[i] = std::toupper(static_cast<unsigned char>(header_name[i]));
        }
        env_vector.push_back("HTTP_" + header_name + "=" + header_val);
    }
    // Really ?
    env_vector.push_back("AUTH_TYPE=");
    env_vector.push_back("GATEWAY_INTERFACE=CGI/1.1");

    env_vector.push_back("REMOTE_ADDR=" + stream.client_host);
    env_vector.push_back("REMOTE_PORT=" + stream.client_port);

    // ill just use the numeric addr
    // env_vector.push_back("REMOTE_HOST=" + stream._host_str);
    // env_vector.push_back("REMOTE_IDENT=");

    env_vector.push_back("REDIRECT_STATUS=200");

    std::vector<char *> env_char;
    for (size_t i = 0; i < env_vector.size(); i++)
        env_char.push_back(const_cast<char *>(env_vector[i].c_str()));
    env_char.push_back(NULL);

    int sv[2]; // sv[0] is for parent, sv[1] is for child
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0)
    {
        std::cerr << "CGI Error: socketpair creation failed." << std::endl;
        stream.setErrorResponse("500", "Internal Server Error");
        _send_header_and_abort = true;
        return;
    }

    _cgi_pid = fork();

    if (_cgi_pid < 0)
    {
        std::cerr << "CGI Error: fork failed." << std::endl;
        close(sv[0]);
        close(sv[1]);
        stream.setErrorResponse("500", "Internal Server Error");
        _send_header_and_abort = true;
        return;
    }

    if (_cgi_pid == 0)
    {
        // FIXME: check return values
        dup2(sv[1], STDIN_FILENO);
        dup2(sv[1], STDOUT_FILENO);

        close(sv[0]);
        close(sv[1]);

        std::vector<char *> argv;
        argv.reserve(_args.size() + 2);
        for (size_t i = 0; i < _args.size(); i++)
            argv.push_back(const_cast<char *>(_args[i].c_str()));
        std::string exec = _root + stream.request.rewrote_URI;
        argv.push_back(const_cast<char *>(exec.c_str()));
        argv.push_back(NULL);
        if (chdir(_root.c_str()) != 0)
        {
            std::cerr << "chdir failed for: " << _root << std::endl;
            std::exit(1);
        }

        execve(argv[0], &argv[0], env_char.data());
        std::cerr << "CGI Error: execve failed for " << argv[0] << ", " << argv[1] << ", "
                  << argv[2] << std::endl;
        std::exit(1);
    }
    close(sv[1]);
    _socket_fd = sv[0];

    if (request_body.size() == 0)
    {
        shutdown(_socket_fd, SHUT_WR);
        _client_body_sent = true;
    }

    _cgi_io = new async::BufIo<>(_reactor, sv[0]);
}

void CGIBackend::send_client_body_to_cgi()
{
    while (!_client_body_sent)
    {
        if (_body_read_buf_index < request_body.size())
        {
            size_t                    write_written;
            container::Slice<uint8_t> buff(&request_body[_body_read_buf_index],
                                           request_body.size() - _body_read_buf_index);
            if (_cgi_io->poll_write(buff, write_written) == Pending)
                return;
            _body_read_buf_index += write_written;
        }
        else
        {
            if (_cgi_io->poll_flush() == Pending)
                return;
            shutdown(_socket_fd, SHUT_WR);
            _client_body_sent = true;
            return;
        }
    }
}

#include "string_overload.hpp"

void CGIBackend::read_cgi_headers()
{
    if (!_response_headers_read_done)
    {
        while (_cgi_io->poll_read_line(header_line) == Ready)
        {
            if ((header_line.size() == 2 && header_line[0] == '\r' && header_line[1] == '\n') ||
                (header_line.size() == 1 && header_line[0] == '\n'))
            {
                if (!_status_set)
                    stream.setHTTPStatus("200", "Ok");
                _response_headers_read_done = true;
                return;
            }
            else
            {
                if (stream.request.http_version != "HTTP/0.9")
                {
                    std::string key, value;
                    if (parse_cgi_header(header_line, key, value))
                    {
                        if (key == "status" && !_status_set)
                        {
                            std::string status_value, status_code;
                            parse_cgi_status_value(value, status_value, status_code);
                            stream.setHTTPStatus(status_value, status_code);
                            _status_set = true;
                        }
                        else if (key == "content-length" && _content_length == -1)
                            _content_length = std::strtoul(value.c_str(), NULL, 10);
                        else
                            stream.response.addHeader(key, value);
                    }
                    else
                    {
                        stream.setErrorResponse("500", "Internal Server Error");
                        _send_header_and_abort = true;
                        return;
                    }
                }
            }
            header_line.clear();
        }
    }
}

async::Future::Poll CGIBackend::set_body_writer()
{
    if (_response_headers_read_done && response_body == NULL)
    {
        if (*&*&_content_length == -1)
        {
            if (stream.poll_send_headers(response_body) == Pending)
                return Pending;
        }
        else
        {
            if (stream.poll_send_headers(_content_length, response_body) == Pending)
                return Pending;
        }
        if (response_body == NULL)
        {
            return Ready;
        }
    }
    return Pending;
}

void CGIBackend::send_body()
{
    if (_response_headers_read_done && response_body)
    {
        while (!_response_body_sent)
        {
            if (_response_buf_index == _response_buf_len)
            {
                container::Slice<uint8_t> buff(_response_buf, 4096);
                if (_cgi_io->poll_read(buff, _response_buf_len) == Pending)
                    return;
                _response_buf_index = 0;
                if (_response_buf_len == 0)
                {
                    shutdown(_socket_fd, SHUT_RD);
                    _response_body_sent = true;
                    return;
                }
            }
            if (_response_buf_index < _response_buf_len)
            {
                size_t                    write_written;
                container::Slice<uint8_t> buff(&_response_buf[_response_buf_index],
                                               _response_buf_len - _response_buf_index);
                if (response_body->poll_write(buff, write_written) == Pending)
                    return;
                _response_buf_index += write_written;
            }
        }
    }
}
