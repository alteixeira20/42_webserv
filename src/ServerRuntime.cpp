#include "ServerRuntime.hpp"

#include <stdexcept>

ServerRuntime::ServerRuntime(void) :
	_listeners(),
	_eventLoop(),
	_clients(),
	_io(),
	_loop(_eventLoop, _clients, _io),
	_running(false)
{
}

ServerRuntime::~ServerRuntime(void)
{
	shutdown();
}

void	ServerRuntime::start(const std::vector<ListenerConfig> &configs)
{
	shutdown();
	_listeners.openAll(configs);
	for (std::size_t i = 0; i < _listeners.listeners().size(); ++i)
		_eventLoop.registerListener(_listeners.listeners()[i].fd);
	_running = true;
}

RuntimeLoopStats	ServerRuntime::runCycle(int timeoutMs)
{
	if (!_running)
		return (RuntimeLoopStats());
	return (_loop.runCycle(timeoutMs));
}

RuntimeLoopStats	ServerRuntime::runCycles(std::size_t cycleCount, int timeoutMs)
{
	RuntimeLoopStats	stats;

	if (!_running)
		return (stats);
	stats = _loop.runCycles(cycleCount, timeoutMs);
	return (stats);
}

RuntimeLoopStats	ServerRuntime::cleanup(std::time_t now,
	std::time_t timeoutSeconds)
{
	if (!_running)
		return (RuntimeLoopStats());
	return (_loop.cleanup(now, timeoutSeconds));
}

void	ServerRuntime::shutdown(void)
{
	_clients.closeAll(_eventLoop);
	for (std::size_t i = 0; i < _listeners.listeners().size(); ++i)
		_eventLoop.unregisterFd(_listeners.listeners()[i].fd);
	_listeners.closeAll();
	_running = false;
}

std::size_t	ServerRuntime::listenerCount(void) const
{
	return (_listeners.listeners().size());
}

std::size_t	ServerRuntime::clientCount(void) const
{
	return (_clients.connections().size());
}

std::size_t	ServerRuntime::watchedCount(void) const
{
	return (_eventLoop.watchedCount());
}

int	ServerRuntime::listenerFd(std::size_t index) const
{
	if (index >= _listeners.listeners().size())
		throw std::out_of_range("listener index out of range");
	return (_listeners.listeners()[index].fd);
}

unsigned short	ServerRuntime::listenerPort(std::size_t index) const
{
	if (index >= _listeners.listeners().size())
		throw std::out_of_range("listener index out of range");
	return (_listeners.listeners()[index].boundPort);
}

ClientManager	&ServerRuntime::clients(void)
{
	return (_clients);
}

const ClientManager	&ServerRuntime::clients(void) const
{
	return (_clients);
}
