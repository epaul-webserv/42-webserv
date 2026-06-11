#pragma once

#include <async>
#include <vector>
#include "ABackend.hpp"

namespace async
{
	template <size_t N1, size_t N2>
	class CopyBufIoToBufIo : public Future
	{
	private:
		BufIo<N1>				 &from;
		BufIo<N2>				 &to;
		bool					  read_done;
		container::Slice<uint8_t> buf;
		size_t					  total;
		size_t					  read;
		CopyBufIoToBufIo();
		CopyBufIoToBufIo(const CopyBufIoToBufIo &other);
		CopyBufIoToBufIo &operator=(const CopyBufIoToBufIo &other);

	public:
		CopyBufIoToBufIo(BufIo<N1> &from, BufIo<N2> &to, size_t total) : from(from), to(to), read_done(false), total(total), read(0) {}
		Future::Poll poll() throw(OsError)
		{
			for (;;)
				if (read_done)
				{
					if (buf.len() == 0)
					{
						return to.poll_flush();
					}
					size_t s;
					if (to.poll_write(buf, s) == Future::Pending)
						return Future::Pending;
					from.consume(s);
					buf = buf.slice(s, buf.len());
				}
				else
				{
					if (buf.len() == 0)
					{
						if (from.poll_fill_buf(buf) == Future::Pending)
							return Future::Pending;
						read += buf.len();
						if (buf.len() == 0 || read >= total)
							read_done = true;
					}
					else
					{
						size_t s;
						if (to.poll_write(buf, s) == Future::Pending)
							return Future::Pending;
						from.consume(s);
						buf = buf.slice(s, buf.len());
					}
				}
		}
		~CopyBufIoToBufIo() {}
	};
} // namespace async

class CGIBackend : public ABackend
{
public:
	CGIBackend(std::vector<std::string> const &args, Request const &request, std::string const &root, Response &response, async::Reactor &reactor, async::BufIo<> &io,
			   std::map<std::string, std::string> const status_pages);
	~CGIBackend(void);

	// void generateResponse(Request const &request, Response &response, async::Reactor &reactor, async::BufIo<> &io);
	Poll poll() throw(async::OsError);

private:
	enum status
	{
		_init,
		_processing,
		_sending
	};
	CGIBackend(const CGIBackend &other);
	CGIBackend &operator=(const CGIBackend &other);


	bool					 parse_cgi_header(const std::vector<uint8_t> &raw_line, std::string &key, std::string &value);
	void					 parse_cgi_status_value(const std::string &status_value, std::string &status_code, std::string &status_message);
	void					 init();
	async::Future::Poll		 copy_to_cgi() throw(async::OsError);
	async::Future::Poll		 process_io() throw(async::OsError);
	std::vector<std::string> _args;
	async::BufIo<>			*_cgi_io;
	size_t					 _body_len;
	std::string				 headers;


	bool _request_body_read_done;
	bool _body_write_done;
	bool _response_headers_read_done;
	bool _status_set;

	uint8_t _request_body_buf[4096];
	size_t	_request_body_read_len;
	size_t	_request_body_read_index;

	uint8_t _response_body_buf[4096];
	size_t	_response_body_read_len;
	size_t	_response_body_read_index;

	status								 _status;
	async::CopyBufIoToBufIo<4096, 4096> *client_body_to_cgi;
};
