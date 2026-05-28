#ifndef CLIENT_IO_HPP
# define CLIENT_IO_HPP

# include "EventLoop.hpp"

# include <cstddef>

struct ClientIoResult
{
	std::size_t	bytes;
	bool		peerClosed;

	ClientIoResult(void);
	ClientIoResult(std::size_t bytes, bool peerClosed);
};

class ClientIo
{
public:
	ClientIo(void);
	explicit ClientIo(std::size_t chunkSize);

	ClientIoResult	handleReadable(const EventLoopEvent &event) const;
	ClientIoResult	handleWritable(const EventLoopEvent &event) const;

private:
	std::size_t	_chunkSize;
};

#endif
