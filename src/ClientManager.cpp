#include "ClientManager.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

static std::string	system_error(const std::string &prefix)
{
	return (prefix + ": " + std::strerror(errno));
}

static void	close_fd(int fd)
{
	if (fd >= 0)
		close(fd);
}

static void	set_non_blocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);

	if (flags < 0)
		throw std::runtime_error(system_error("fcntl(F_GETFL) failed"));
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
		throw std::runtime_error(system_error("fcntl(F_SETFL, O_NONBLOCK) failed"));
}

ClientManager::ClientManager(void)
{
}

ClientManager::~ClientManager(void)
{
	closeAll();
}

std::size_t	ClientManager::acceptReady(const EventLoopEvent &event,
	EventLoop &eventLoop)
{
	std::size_t	accepted;

	accepted = 0;
	if (event.kind != EventLoopEvent::LISTENER || !event.readable)
		return (accepted);
	while (true)
	{
		int clientFd = accept(event.fd, NULL, NULL);

		if (clientFd < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break ;
			throw std::runtime_error(system_error("accept failed"));
		}
		try
		{
			addClient(clientFd, event.fd, eventLoop);
			++accepted;
		}
		catch (...)
		{
			close_fd(clientFd);
			throw;
		}
	}
	return (accepted);
}

std::size_t	ClientManager::closeTimedOut(EventLoop &eventLoop,
	std::time_t now, std::time_t timeoutSeconds)
{
	ConnectionList::iterator	it;
	std::size_t				removed;

	removed = 0;
	it = _connections.begin();
	while (it != _connections.end())
	{
		if (it->isTimedOut(now, timeoutSeconds))
		{
			it->closeWithReason("client timeout");
			it = removeClient(it, eventLoop);
			++removed;
		}
		else
			++it;
	}
	return (removed);
}

std::size_t	ClientManager::removeClosing(EventLoop &eventLoop)
{
	ConnectionList::iterator	it;
	std::size_t				removed;

	removed = 0;
	it = _connections.begin();
	while (it != _connections.end())
	{
		if (it->state() == ClientConnection::CLOSING)
		{
			it = removeClient(it, eventLoop);
			++removed;
		}
		else
			++it;
	}
	return (removed);
}

void	ClientManager::closeAll(void)
{
	for (ConnectionList::iterator it = _connections.begin();
		it != _connections.end(); ++it)
		close_fd(it->fd());
	_connections.clear();
}

void	ClientManager::closeAll(EventLoop &eventLoop)
{
	for (ConnectionList::iterator it = _connections.begin();
		it != _connections.end(); ++it)
	{
		eventLoop.unregisterFd(it->fd());
		close_fd(it->fd());
	}
	_connections.clear();
}

ClientManager::ConnectionList	&ClientManager::connections(void)
{
	return (_connections);
}

const ClientManager::ConnectionList	&ClientManager::connections(void) const
{
	return (_connections);
}

void	ClientManager::addClient(int clientFd, int listenerFd, EventLoop &eventLoop)
{
	set_non_blocking(clientFd);
	_connections.push_back(ClientConnection(clientFd, listenerFd));
	eventLoop.registerClient(_connections.back());
}

ClientManager::ConnectionList::iterator	ClientManager::removeClient(
	ConnectionList::iterator it, EventLoop &eventLoop)
{
	eventLoop.unregisterFd(it->fd());
	close_fd(it->fd());
	return (_connections.erase(it));
}
