#include "ClientConnection.hpp"

ClientConnection::ClientConnection(int fd, int listenerFd) :
	_fd(fd),
	_listenerFd(listenerFd),
	_state(READING_HEADERS),
	_readBuffer(),
	_writeBuffer(),
	_closeReason(),
	_lastActivity(std::time(NULL))
{
}

int	ClientConnection::fd(void) const
{
	return (_fd);
}

int	ClientConnection::listenerFd(void) const
{
	return (_listenerFd);
}

ClientConnection::State	ClientConnection::state(void) const
{
	return (_state);
}

const std::string	&ClientConnection::readBuffer(void) const
{
	return (_readBuffer);
}

const std::string	&ClientConnection::writeBuffer(void) const
{
	return (_writeBuffer);
}

const std::string	&ClientConnection::closeReason(void) const
{
	return (_closeReason);
}

std::time_t	ClientConnection::lastActivity(void) const
{
	return (_lastActivity);
}

void	ClientConnection::setState(State state)
{
	_state = state;
	touch();
}

void	ClientConnection::appendReadData(const std::string &data)
{
	_readBuffer += data;
	touch();
}

void	ClientConnection::appendWriteData(const std::string &data)
{
	_writeBuffer += data;
	touch();
}

void	ClientConnection::consumeWrittenBytes(std::size_t count)
{
	if (count >= _writeBuffer.size())
		_writeBuffer.clear();
	else
		_writeBuffer.erase(0, count);
	touch();
}

void	ClientConnection::closeWithReason(const std::string &reason)
{
	_closeReason = reason;
	_state = CLOSING;
	touch();
}

void	ClientConnection::touch(void)
{
	_lastActivity = std::time(NULL);
}

bool	ClientConnection::isTimedOut(std::time_t now,
	std::time_t timeoutSeconds) const
{
	if (now < _lastActivity)
		return (false);
	return ((now - _lastActivity) >= timeoutSeconds);
}

bool	ClientConnection::wantsRead(void) const
{
	return (_state == READING_HEADERS || _state == READING_BODY);
}

bool	ClientConnection::wantsWrite(void) const
{
	return (_state == WRITING_RESPONSE && !_writeBuffer.empty());
}
