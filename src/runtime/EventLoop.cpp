#include "runtime/EventLoop.hpp"

#include <cerrno>
#include <cstring>
#include <poll.h>
#include <stdexcept>
#include <string>

static std::string	poll_error(void)
{
	return (std::string("poll failed: ") + std::strerror(errno));
}

EventLoopEvent::EventLoopEvent(void) :
	fd(-1),
	kind(LISTENER),
	client(NULL),
	readable(false),
	writable(false),
	closed(false),
	error(false)
{
}

EventLoop::EventLoop(void)
{
}

void	EventLoop::registerListener(int fd)
{
	Watch watch;

	watch.fd = fd;
	watch.kind = EventLoopEvent::LISTENER;
	watch.client = NULL;
	watch.watchRead = true;
	watch.watchWrite = false;
	registerWatch(watch);
}

void	EventLoop::registerClient(ClientConnection &client)
{
	Watch watch;

	watch.fd = client.fd();
	watch.kind = EventLoopEvent::CLIENT;
	watch.client = &client;
	watch.watchRead = false;
	watch.watchWrite = false;
	registerWatch(watch);
}

void	EventLoop::registerCgiPipe(int fd, bool watchRead, bool watchWrite)
{
	Watch watch;

	watch.fd = fd;
	watch.kind = EventLoopEvent::CGI_PIPE;
	watch.client = NULL;
	watch.watchRead = watchRead;
	watch.watchWrite = watchWrite;
	registerWatch(watch);
}

void	EventLoop::unregisterFd(int fd)
{
	for (std::vector<Watch>::iterator it = _watches.begin();
		it != _watches.end(); ++it)
	{
		if (it->fd == fd)
		{
			_watches.erase(it);
			return ;
		}
	}
}

std::vector<EventLoopEvent>	EventLoop::pollOnce(int timeoutMs)
{
	std::vector<pollfd>			pollFds;
	std::vector<EventLoopEvent>	events;
	int							ready;

	pollFds.reserve(_watches.size());
	for (std::size_t i = 0; i < _watches.size(); ++i)
	{
		pollfd pollFd;

		pollFd.fd = _watches[i].fd;
		pollFd.events = eventsForWatch(_watches[i]);
		pollFd.revents = 0;
		if (pollFd.events != 0)
			pollFds.push_back(pollFd);
	}
	if (pollFds.empty())
		return (events);
	ready = poll(&pollFds[0], pollFds.size(), timeoutMs);
	if (ready < 0)
		throw std::runtime_error(poll_error());
	for (std::size_t i = 0; i < pollFds.size(); ++i)
	{
		if (pollFds[i].revents == 0)
			continue ;
		EventLoopEvent event;

		event.fd = pollFds[i].fd;
		for (std::size_t j = 0; j < _watches.size(); ++j)
		{
			if (_watches[j].fd == event.fd)
			{
				event.kind = _watches[j].kind;
				event.client = _watches[j].client;
				break ;
			}
		}
		event.readable = (pollFds[i].revents & POLLIN);
		event.writable = (pollFds[i].revents & POLLOUT);
		event.closed = (pollFds[i].revents & POLLHUP);
		event.error = (pollFds[i].revents & (POLLERR | POLLNVAL));
		events.push_back(event);
	}
	return (events);
}

std::size_t	EventLoop::watchedCount(void) const
{
	return (_watches.size());
}

void	EventLoop::registerWatch(const Watch &watch)
{
	unregisterFd(watch.fd);
	_watches.push_back(watch);
}

short	EventLoop::eventsForWatch(const Watch &watch) const
{
	short events;

	events = 0;
	if (watch.kind == EventLoopEvent::LISTENER)
		events |= POLLIN;
	else if (watch.kind == EventLoopEvent::CLIENT && watch.client != NULL)
	{
		if (watch.client->wantsRead())
			events |= POLLIN;
		if (watch.client->wantsWrite())
			events |= POLLOUT;
	}
	else
	{
		if (watch.watchRead)
			events |= POLLIN;
		if (watch.watchWrite)
			events |= POLLOUT;
	}
	return (events);
}
