#ifndef CLIENT_CONNECTION_HPP
# define CLIENT_CONNECTION_HPP

# include <ctime>
# include <string>

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

	int					fd(void) const;
	int					listenerFd(void) const;
	State				state(void) const;
	const std::string	&readBuffer(void) const;
	const std::string	&writeBuffer(void) const;
	const std::string	&closeReason(void) const;
	std::time_t			lastActivity(void) const;

	void	setState(State state);
	void	appendReadData(const std::string &data);
	void	appendWriteData(const std::string &data);
	void	consumeWrittenBytes(std::size_t count);
	void	closeWithReason(const std::string &reason);
	void	touch(void);

	bool	isTimedOut(std::time_t now, std::time_t timeoutSeconds) const;
	bool	wantsRead(void) const;
	bool	wantsWrite(void) const;

private:
	int			_fd;
	int			_listenerFd;
	State		_state;
	std::string	_readBuffer;
	std::string	_writeBuffer;
	std::string	_closeReason;
	std::time_t	_lastActivity;
};

#endif
