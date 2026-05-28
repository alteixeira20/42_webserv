#ifndef SERVER_RUNTIME_HPP
# define SERVER_RUNTIME_HPP

# include "ClientIo.hpp"
# include "ClientManager.hpp"
# include "EventLoop.hpp"
# include "ListenerConfig.hpp"
# include "ListenerManager.hpp"
# include "RuntimeLoop.hpp"

# include <cstddef>
# include <ctime>
# include <vector>

class ServerRuntime
{
public:
	ServerRuntime(void);
	~ServerRuntime(void);

	void				start(const std::vector<ListenerConfig> &configs);
	RuntimeLoopStats	runCycle(int timeoutMs);
	RuntimeLoopStats	runCycles(std::size_t cycleCount, int timeoutMs);
	RuntimeLoopStats	cleanup(std::time_t now, std::time_t timeoutSeconds);
	void				shutdown(void);

	std::size_t			listenerCount(void) const;
	std::size_t			clientCount(void) const;
	std::size_t			watchedCount(void) const;
	int					listenerFd(std::size_t index) const;
	unsigned short		listenerPort(std::size_t index) const;
	ClientManager		&clients(void);
	const ClientManager	&clients(void) const;

private:
	ListenerManager	_listeners;
	EventLoop		_eventLoop;
	ClientManager	_clients;
	ClientIo		_io;
	RuntimeLoop		_loop;
	bool			_running;

	ServerRuntime(const ServerRuntime &other);
	ServerRuntime	&operator=(const ServerRuntime &other);
};

#endif
