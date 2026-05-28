#include "runtime/ClientManager.hpp"
#include "runtime/EventLoop.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

static void	assert_true(bool condition, const std::string &message)
{
	if (!condition)
		throw std::runtime_error(message);
}

static void	close_fd(int fd)
{
	if (fd >= 0)
		close(fd);
}

static void	set_non_blocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);

	if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl failed");
}

static bool	is_non_blocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);

	return (flags >= 0 && (flags & O_NONBLOCK));
}

static int	create_listener(unsigned short &port)
{
	int					fd;
	int					enabled;
	sockaddr_in			address;
	socklen_t			addressLength;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		throw std::runtime_error("socket failed");
	enabled = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled)) < 0)
		throw std::runtime_error("setsockopt failed");
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	address.sin_port = 0;
	if (bind(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0)
		throw std::runtime_error("bind failed");
	if (listen(fd, 16) < 0)
		throw std::runtime_error("listen failed");
	set_non_blocking(fd);
	addressLength = sizeof(address);
	if (getsockname(fd, reinterpret_cast<sockaddr *>(&address),
			&addressLength) < 0)
		throw std::runtime_error("getsockname failed");
	port = ntohs(address.sin_port);
	return (fd);
}

static int	connect_client(unsigned short port)
{
	int			fd;
	sockaddr_in	address;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		throw std::runtime_error("client socket failed");
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	address.sin_port = htons(port);
	if (connect(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0)
		throw std::runtime_error("connect failed");
	return (fd);
}

static EventLoopEvent	first_listener_event(EventLoop &loop)
{
	std::vector<EventLoopEvent> events = loop.pollOnce(100);

	for (std::size_t i = 0; i < events.size(); ++i)
	{
		if (events[i].kind == EventLoopEvent::LISTENER && events[i].readable)
			return (events[i]);
	}
	throw std::runtime_error("listener was not ready");
}

static void	test_accepts_all_pending_clients_and_registers_them()
{
	unsigned short	port;
	int				listenerFd;
	int				clientOne;
	int				clientTwo;
	EventLoop		loop;
	ClientManager	manager;

	listenerFd = create_listener(port);
	clientOne = connect_client(port);
	clientTwo = connect_client(port);
	loop.registerListener(listenerFd);

	std::size_t accepted = manager.acceptReady(first_listener_event(loop), loop);

	assert_true(accepted == 2, "expected all pending clients to be accepted");
	assert_true(manager.connections().size() == 2,
		"expected accepted clients to be stored");
	assert_true(loop.watchedCount() == 3,
		"expected listener plus two clients to be watched");
	for (ClientManager::ConnectionList::const_iterator it =
			manager.connections().begin(); it != manager.connections().end(); ++it)
	{
		assert_true(it->listenerFd() == listenerFd,
			"expected accepted client to remember listener fd");
		assert_true(it->state() == ClientConnection::READING_HEADERS,
			"expected accepted client to start reading headers");
		assert_true(is_non_blocking(it->fd()),
			"expected accepted client fd to be non-blocking");
	}
	close_fd(clientOne);
	close_fd(clientTwo);
	close_fd(listenerFd);
}

static void	test_accepted_clients_are_polled_for_reads()
{
	unsigned short	port;
	int				listenerFd;
	int				clientFd;
	EventLoop		loop;
	ClientManager	manager;

	listenerFd = create_listener(port);
	clientFd = connect_client(port);
	loop.registerListener(listenerFd);
	manager.acceptReady(first_listener_event(loop), loop);
	write(clientFd, "G", 1);

	std::vector<EventLoopEvent> events = loop.pollOnce(100);

	close_fd(clientFd);
	close_fd(listenerFd);
	for (std::size_t i = 0; i < events.size(); ++i)
	{
		if (events[i].kind == EventLoopEvent::CLIENT && events[i].readable)
			return ;
	}
	throw std::runtime_error("expected accepted client to be readable in poll");
}

int	main(void)
{
	try
	{
		test_accepts_all_pending_clients_and_registers_them();
		test_accepted_clients_are_polled_for_reads();
	}
	catch (const std::exception &error)
	{
		std::cerr << "test_client_manager: " << error.what() << std::endl;
		return (1);
	}
	std::cout << "test_client_manager: OK" << std::endl;
	return (0);
}
