#include "../includes/FdStream.hpp"

FdStream::FdStream(int fd) : _end(false), _fd(fd), _buffer(), _read_index(0), _write_index(0) {}

FdStream::~FdStream()
{
    if (this->_fd > 2)
        close(this->_fd);
}
FdStream &FdStream::operator=(FdStream const &other)
{
    if (this != &other)
    {
        this->_end = other._end;
        this->_fd  = other._fd;
        std::copy(other._buffer, other._buffer + other._write_index, this->_buffer);
        this->_read_index  = other._read_index;
        this->_write_index = other._write_index;
    }
    return *this;
}

bool FdStream::end() const
{
    if (this->_end && this->_write_index - this->_read_index == 0)
        return (true);
    return (false);
}

#include <iostream>

void FdStream::consume(size_t n) { this->_read_index += n; }

std::vector<uint8_t> FdStream::peek_str(size_t n)
{
    size_t sent_bytes = 0;

    if (this->_read_index - this->_write_index >= n)
    {
        std::vector<uint8_t> ret(this->_buffer + this->_read_index,
                                 this->_buffer + this->_read_index +
                                     std::min(this->_write_index - this->_read_index, n));
        return (ret);
    }
    std::vector<uint8_t> ret;
    while (ret.size() < n)
    {
        if (this->_end && this->_read_index + sent_bytes == this->_write_index)
            break;
        if (this->_write_index == 0 || this->_read_index + sent_bytes == FDSTREAM_BUFF_SIZE)
            fillBuffer();
        size_t to_write = std::min(this->_write_index - this->_read_index - sent_bytes, n);
        ret.insert(ret.end(), this->_buffer + this->_read_index,
                   this->_buffer + this->_read_index + to_write);
        sent_bytes += to_write;
    }
    return (ret);
}

std::string FdStream::get_str(size_t n)
{
    if (this->_read_index - this->_write_index >= n)
    {
        std::string ret(this->_buffer + this->_read_index, n);
        this->_read_index += n;
        return (ret);
    }
    std::string ret;
    while (ret.size() < n)
    {
        if (this->_end && this->_read_index == this->_write_index)
            break;
        if (this->_write_index == 0 || this->_read_index == FDSTREAM_BUFF_SIZE)
            fillBuffer();
        size_t to_write = std::min(this->_write_index - this->_read_index, n);
        ret.append(this->_buffer + this->_read_index, to_write);
        this->_read_index += to_write;
    }
    return (ret);
}

std::string FdStream::get_str_until(char c)
{
    char const *char_pos = NULL;
    std::string ret;
    size_t      to_write;

    while (!char_pos)
    {
        if (this->_end && this->_read_index == this->_write_index)
            break;
        if (this->_write_index == 0 || this->_read_index == this->_write_index)
            fillBuffer();
        if (this->_end && !this->_write_index)
            break;
        char_pos = strchr(this->_buffer + this->_read_index, c);
        if (!char_pos)
            to_write = this->_write_index - this->_read_index;
        else
            to_write = std::min(char_pos - this->_buffer - this->_read_index,
                                this->_write_index - this->_read_index);
        ret.append(this->_buffer + this->_read_index, to_write);
        this->_read_index += to_write + (char_pos ? 1 : 0);
    }
    return (ret);
}

bool FdStream::is_open() const { return (this->_fd != -1); }

void FdStream::clear(int fd)
{
    this->_end = false;
    if (this->_fd > 2)
        close(this->_fd);
    this->_fd          = fd;
    this->_read_index  = 0;
    this->_write_index = 0;
}

void FdStream::fillBuffer()
{
    if (this->_write_index == FDSTREAM_BUFF_SIZE)
        this->_write_index = 0;
    long read_bytes = read(this->_fd, this->_buffer, FDSTREAM_BUFF_SIZE - this->_write_index);

    if (read_bytes == -1 && errno & EBADF)
        throw(InvalidFdException());
    if (read_bytes == -1)
        throw(InvalidReadException());
    if (read_bytes == 0 ||
        static_cast<unsigned int>(read_bytes) < FDSTREAM_BUFF_SIZE - this->_write_index)
        this->_end = true;
    this->_buffer[read_bytes] = '\0';
    this->_write_index        = read_bytes;
    this->_read_index         = 0;
}

const char *FdStream::InvalidFdException::what() const throw() { return ("Invalid fd ?"); }

const char *FdStream::InvalidReadException::what() const throw() { return ("Error while reading"); }
