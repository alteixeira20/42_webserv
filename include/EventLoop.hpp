#ifndef EVENT_LOOP_HPP
# define EVENT_LOOP_HPP

# include "ClientConnection.hpp"

# include <vector>

struct EventLoopEvent
{
	enum Kind
	{
		LISTENER,
		CLIENT,
		CGI_PIPE
	};

	int					fd;
	Kind				kind;
	ClientConnection	*client;
	bool				readable;
	bool				writable;
	bool				closed;
	bool				error;

	EventLoopEvent(void);
};

class EventLoop
{
public:
	EventLoop(void);

	void	registerListener(int fd);
	void	registerClient(ClientConnection &client);
	void	registerCgiPipe(int fd, bool watchRead, bool watchWrite);
	void	unregisterFd(int fd);

	std::vector<EventLoopEvent>	pollOnce(int timeoutMs);
	std::size_t					watchedCount(void) const;

private:
	struct Watch
	{
		int							fd;
		EventLoopEvent::Kind		kind;
		ClientConnection			*client;
		bool						watchRead;
		bool						watchWrite;
	};

	std::vector<Watch>	_watches;

	EventLoop(const EventLoop &other);
	EventLoop	&operator=(const EventLoop &other);

	void	registerWatch(const Watch &watch);
	short	eventsForWatch(const Watch &watch) const;
};

#endif
