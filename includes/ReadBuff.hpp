#ifndef WEBSERV_READBUFF_HPP
#define WEBSERV_READBUFF_HPP

#include <cstddef>

template <typename T> class ReadBuff {
  public:
    ReadBuff(void);
    ReadBuff(size_t size);
    ReadBuff(ReadBuff const &ReadBuff);
    ~ReadBuff(void);

    Readbuff const &operator=(ReadBuff const &ReadBuff);
    char *unfilled();
    char *filled;

  private:
    const size_t _BUFF_SIZE;
    T _buffer[_BUFF_SIZE];
    size_t _read_index;
    size_t _write_index;
};

#endif
