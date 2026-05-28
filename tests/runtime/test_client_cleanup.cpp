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

static int	accept_one_client(ClientManager &manager, EventLoop &loop, int peerFd)
{
	int acceptedFd;

	(void)peerFd;
	manager.acceptReady(first_listener_event(loop), loop);
	assert_true(manager.connections().size() == 1,
		"expected one accepted client");
	acceptedFd = manager.connections().front().fd();
	assert_true(fcntl(acceptedFd, F_GETFL, 0) >= 0,
		"expected accepted fd to be open before cleanup");
	return (acceptedFd);
}

static void	test_timeout_closes_client_and_unregisters_fd()
{
	unsigned short	port;
	int				listenerFd;
	int				peerFd;
	int				acceptedFd;
	EventLoop		loop;
	ClientManager	manager;

	listenerFd = create_listener(port);
	peerFd = connect_client(port);
	loop.registerListener(listenerFd);
	acceptedFd = accept_one_client(manager, loop, peerFd);

	std::size_t removed = manager.closeTimedOut(loop, std::time(NULL), 0);

	close_fd(peerFd);
	close_fd(listenerFd);
	assert_true(removed == 1, "expected timeout sweep to remove one client");
	assert_true(manager.connections().empty(),
		"expected timed-out client to be erased");
	assert_true(loop.watchedCount() == 1,
		"expected only listener to remain watched");
	assert_true(fcntl(acceptedFd, F_GETFL, 0) < 0,
		"expected timed-out client fd to be closed");
}

static void	test_closing_client_cleanup_does_not_touch_active_clients()
{
	unsigned short	port;
	int				listenerFd;
	int				peerOne;
	int				peerTwo;
	EventLoop		loop;
	ClientManager	manager;

	listenerFd = create_listener(port);
	peerOne = connect_client(port);
	peerTwo = connect_client(port);
	loop.registerListener(listenerFd);
	manager.acceptReady(first_listener_event(loop), loop);
	assert_true(manager.connections().size() == 2,
		"expected two accepted clients");
	manager.connections().front().closeWithReason("bad request");

	std::size_t removed = manager.removeClosing(loop);

	close_fd(peerOne);
	close_fd(peerTwo);
	close_fd(listenerFd);
	assert_true(removed == 1, "expected one closing client to be removed");
	assert_true(manager.connections().size() == 1,
		"expected active client to remain");
	assert_true(loop.watchedCount() == 2,
		"expected listener plus active client to remain watched");
}

static void	test_close_all_unregisters_and_closes_clients()
{
	unsigned short	port;
	int				listenerFd;
	int				peerOne;
	int				peerTwo;
	int				firstAcceptedFd;
	int				secondAcceptedFd;
	EventLoop		loop;
	ClientManager	manager;

	listenerFd = create_listener(port);
	peerOne = connect_client(port);
	peerTwo = connect_client(port);
	loop.registerListener(listenerFd);
	manager.acceptReady(first_listener_event(loop), loop);
	assert_true(manager.connections().size() == 2,
		"expected two accepted clients before closeAll");
	firstAcceptedFd = manager.connections().front().fd();
	secondAcceptedFd = manager.connections().back().fd();

	manager.closeAll(loop);

	close_fd(peerOne);
	close_fd(peerTwo);
	close_fd(listenerFd);
	assert_true(manager.connections().empty(),
		"expected closeAll to erase all clients");
	assert_true(loop.watchedCount() == 1,
		"expected only listener to remain watched after closeAll");
	assert_true(fcntl(firstAcceptedFd, F_GETFL, 0) < 0,
		"expected first accepted fd to be closed by closeAll");
	assert_true(fcntl(secondAcceptedFd, F_GETFL, 0) < 0,
		"expected second accepted fd to be closed by closeAll");
}

int	main(void)
{
	try
	{
		test_timeout_closes_client_and_unregisters_fd();
		test_closing_client_cleanup_does_not_touch_active_clients();
		test_close_all_unregisters_and_closes_clients();
	}
	catch (const std::exception &error)
	{
		std::cerr << "test_client_cleanup: " << error.what() << std::endl;
		return (1);
	}
	std::cout << "test_client_cleanup: OK" << std::endl;
	return (0);
}
