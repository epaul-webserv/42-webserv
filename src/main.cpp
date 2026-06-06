#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

#include <async>

#include "common_header.hpp"
#include "request.hpp"

class RequestParser : public async::Future {
    async::BufIo<> in;
    RequestData data;
    bool first;
    std::vector<uint8_t> line;

  public:
    RequestParser(async::Reactor &reactor, int in)
        : in(reactor, in), first(true) {}
    Poll poll() {
        for (;;) {
            line.clear();
            if (in.poll_read_line(line) == Pending)
                return Pending;
            if (line.empty())
                return Ready;
            if (first) {
                first = false;
                continue;
            }
            std::vector<uint8_t>::iterator col =
                std::find(line.begin(), line.end(), ':');
            if (col == line.end())
                throw std::runtime_error("no `:` in header line");
            CommonHeader header;
            if (CommonHeader::try_from_string(line.data(), col - line.begin(),
                                              header))
                std::cout << header.as_cstr() << '\n';
            else
                std::cout << "Unrecognized header: "
                          << std::string((char *)line.data(),
                                         col - line.begin())
                          << '\n';
        }
    }

    ~RequestParser() {}
};

int main() {
    async::Reactor reactor;
    RequestParser parser(reactor, STDIN_FILENO);
    reactor.run(parser);
}
