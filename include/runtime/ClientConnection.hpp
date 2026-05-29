#ifndef CLIENT_CONNECTION_HPP
# define CLIENT_CONNECTION_HPP

# include <ctime>
# include <string>
# include "http/HttpRequestParser.hpp"

class ClientConnection
{
public:
	enum State
	{
		READING_HEADERS,
		READING_BODY,
		PROCESSING,
		CGI_RUNNING,
		WRITING_RESPONSE,
		CLOSING
	};

	ClientConnection(int fd, int listenerFd);

	int						fd() const;
	int						listenerFd() const;
	State					state() const;
	const std::string		&readBuffer() const;
	const std::string		&writeBuffer() const;
	const std::string		&closeReason() const;
	std::time_t				lastActivity() const;
	bool					hasParsedRequest() const;
	bool					hasParserError() const;
	unsigned int			parserErrorStatus() const;
	const HttpRequest		&getParsedRequest() const;
	const HttpRequestParser	&getRequestParser() const;

	void	setState(State state);
	void	appendReadData(const std::string &data);
	void	appendWriteData(const std::string &data);
	void	consumeWrittenBytes(std::size_t count);
	void	closeWithReason(const std::string &reason);
	void	touch();

	bool	isTimedOut(std::time_t now, std::time_t timeoutSeconds) const;
	bool	wantsRead() const;
	bool	wantsWrite() const;

private:
	int					_fd;
	int					_listenerFd;
	State				_state;
	std::string			_readBuffer;
	std::string			_writeBuffer;
	std::string			_closeReason;
	std::time_t			_lastActivity;
	HttpRequestParser	_requestParser;

	void	parseReadData(const std::string &data);
};

#endif
