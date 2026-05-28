#include "runtime/ClientIo.hpp"

#include <cerrno>
#include <string>
#include <unistd.h>
#include <vector>

ClientIoResult::ClientIoResult(void) :
	bytes(0),
	peerClosed(false)
{
}

ClientIoResult::ClientIoResult(std::size_t bytes, bool peerClosed) :
	bytes(bytes),
	peerClosed(peerClosed)
{
}

ClientIo::ClientIo(void) :
	_chunkSize(8192)
{
}

ClientIo::ClientIo(std::size_t chunkSize) :
	_chunkSize(chunkSize == 0 ? 8192 : chunkSize)
{
}

ClientIoResult	ClientIo::handleReadable(const EventLoopEvent &event) const
{
	std::vector<char>	buffer;
	ssize_t				count;

	if (event.kind != EventLoopEvent::CLIENT || event.client == NULL
		|| !event.readable || !event.client->wantsRead())
		return (ClientIoResult());
	buffer.resize(_chunkSize);
	count = read(event.fd, &buffer[0], buffer.size());
	if (count > 0)
	{
		event.client->appendReadData(std::string(&buffer[0], count));
		return (ClientIoResult(static_cast<std::size_t>(count), false));
	}
	if (count == 0)
	{
		event.client->closeWithReason("client closed connection");
		return (ClientIoResult(0, true));
	}
	if (errno != EAGAIN && errno != EWOULDBLOCK)
		event.client->closeWithReason("client read error");
	return (ClientIoResult());
}

ClientIoResult	ClientIo::handleWritable(const EventLoopEvent &event) const
{
	std::size_t	toWrite;
	ssize_t		count;

	if (event.kind != EventLoopEvent::CLIENT || event.client == NULL
		|| !event.writable || !event.client->wantsWrite())
		return (ClientIoResult());
	toWrite = event.client->writeBuffer().size();
	if (toWrite > _chunkSize)
		toWrite = _chunkSize;
	count = write(event.fd, event.client->writeBuffer().c_str(), toWrite);
	if (count > 0)
	{
		event.client->consumeWrittenBytes(static_cast<std::size_t>(count));
		return (ClientIoResult(static_cast<std::size_t>(count), false));
	}
	if (count < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
		event.client->closeWithReason("client write error");
	return (ClientIoResult());
}
