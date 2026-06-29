#include "runtime/ClientConnection.hpp"

ClientConnection::ClientConnection(int fd, int listenerFd) :
	_fd(fd),
	_listenerFd(listenerFd),
	_state(READING_HEADERS),
	_readBuffer(),
	_writeBuffer(),
	_closeReason(),
	_lastActivity(std::time(NULL)),
	_requestParser()
{
}

int	ClientConnection::fd() const
{
	return (_fd);
}

int	ClientConnection::listenerFd() const
{
	return (_listenerFd);
}

ClientConnection::State	ClientConnection::state() const
{
	return (_state);
}

const std::string	&ClientConnection::readBuffer() const
{
	return (_readBuffer);
}

const std::string	&ClientConnection::writeBuffer() const
{
	return (_writeBuffer);
}

const std::string	&ClientConnection::closeReason() const
{
	return (_closeReason);
}

std::time_t	ClientConnection::lastActivity() const
{
	return (_lastActivity);
}

bool	ClientConnection::hasParsedRequest() const
{
	return (_requestParser.isComplete());
}

bool	ClientConnection::hasParserError() const
{
	return (_requestParser.hasError());
}

unsigned int	ClientConnection::parserErrorStatus() const
{
	return (_requestParser.getErrorStatus());
}

const HttpRequest	&ClientConnection::getParsedRequest() const
{
	return (_requestParser.getRequest());
}

const HttpRequestParser	&ClientConnection::getRequestParser() const
{
	return (_requestParser);
}

void	ClientConnection::setState(State state)
{
	_state = state;
	touch();
}

void	ClientConnection::appendReadData(const std::string &data)
{
	_readBuffer += data;
	if (_state == READING_HEADERS || _state == READING_BODY)
		parseReadData(data);
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

void	ClientConnection::touch()
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

bool	ClientConnection::wantsRead() const
{
	return (_state == READING_HEADERS || _state == READING_BODY);
}

bool	ClientConnection::wantsWrite() const
{
	return (_state == WRITING_RESPONSE && !_writeBuffer.empty());
}

void	ClientConnection::parseReadData(const std::string &data)
{
	if (data.empty())
		return ;
	_requestParser.append(data);
	if (_requestParser.hasError())
		closeWithReason("http request parse error");
	else if (_requestParser.isComplete())
		setState(PROCESSING);
	else if (_requestParser.getState() == HttpRequestParser::PARSING_BODY
		|| _requestParser.getState() == HttpRequestParser::PARSING_CHUNKED_BODY)
		setState(READING_BODY);
}
