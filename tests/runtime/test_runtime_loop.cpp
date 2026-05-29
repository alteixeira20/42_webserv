#include "runtime/RuntimeLoop.hpp"

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

static void	assert_stats_zero(const RuntimeLoopStats &stats,
	const std::string &message)
{
	assert_true(stats.cycles == 0, message);
	assert_true(stats.listenerEvents == 0, message);
	assert_true(stats.clientEvents == 0, message);
	assert_true(stats.acceptedClients == 0, message);
	assert_true(stats.bytesRead == 0, message);
	assert_true(stats.bytesWritten == 0, message);
	assert_true(stats.removedClients == 0, message);
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

static std::string	read_available(int fd)
{
	char		buffer[32];
	ssize_t		count;
	std::string	result;

	while (true)
	{
		count = read(fd, buffer, sizeof(buffer));
		if (count > 0)
			result.append(buffer, count);
		else
			return (result);
	}
}

static void	test_stats_default_values_are_zero(void)
{
	RuntimeLoopStats stats;

	assert_stats_zero(stats, "expected default runtime loop stats to be zero");
}

static void	test_stats_add_accumulates_values(void)
{
	RuntimeLoopStats	first;
	RuntimeLoopStats	second;

	first.cycles = 1;
	first.listenerEvents = 2;
	first.clientEvents = 3;
	first.acceptedClients = 4;
	first.bytesRead = 5;
	first.bytesWritten = 6;
	first.removedClients = 7;
	second.cycles = 10;
	second.listenerEvents = 20;
	second.clientEvents = 30;
	second.acceptedClients = 40;
	second.bytesRead = 50;
	second.bytesWritten = 60;
	second.removedClients = 70;
	first.add(second);
	assert_true(first.cycles == 11, "expected cycles to accumulate");
	assert_true(first.listenerEvents == 22,
		"expected listener events to accumulate");
	assert_true(first.clientEvents == 33,
		"expected client events to accumulate");
	assert_true(first.acceptedClients == 44,
		"expected accepted clients to accumulate");
	assert_true(first.bytesRead == 55, "expected read bytes to accumulate");
	assert_true(first.bytesWritten == 66,
		"expected written bytes to accumulate");
	assert_true(first.removedClients == 77,
		"expected removed clients to accumulate");
}

static void	test_one_cycle_handles_ready_read_and_write_clients(void)
{
	int					readFds[2];
	int					writeFds[2];
	EventLoop			eventLoop;
	ClientManager		clients;
	ClientIo			io(3);
	RuntimeLoop			runtime(eventLoop, clients, io);

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, readFds) != 0)
		throw std::runtime_error("socketpair failed");
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, writeFds) != 0)
		throw std::runtime_error("socketpair failed");
	set_non_blocking(readFds[0]);
	set_non_blocking(readFds[1]);
	set_non_blocking(writeFds[0]);
	set_non_blocking(writeFds[1]);
	clients.connections().push_back(ClientConnection(readFds[0], 10));
	eventLoop.registerClient(clients.connections().back());
	clients.connections().push_back(ClientConnection(writeFds[0], 10));
	clients.connections().back().setState(ClientConnection::WRITING_RESPONSE);
	clients.connections().back().appendWriteData("abcdef");
	eventLoop.registerClient(clients.connections().back());
	write(readFds[1], "123456", 6);

	RuntimeLoopStats stats = runtime.runCycle(100);
	std::string firstWrite = read_available(writeFds[1]);

	assert_true(stats.clientEvents == 2,
		"expected two client events in first cycle");
	assert_true(stats.bytesRead == 3,
		"expected one read chunk in first cycle");
	assert_true(stats.bytesWritten == 3,
		"expected one write chunk in first cycle");
	assert_true(firstWrite == "abc", "expected first write chunk only");
	assert_true(clients.connections().front().readBuffer() == "123",
		"expected one read chunk to be buffered");
	assert_true(clients.connections().back().writeBuffer() == "def",
		"expected one write chunk to remain buffered");

	clients.closeAll(eventLoop);
	close_fd(readFds[1]);
	close_fd(writeFds[1]);
	assert_true(clients.connections().empty(),
		"expected test cleanup to remove clients");
}

static void	test_run_cycles_repeats_poll_and_cleanup(void)
{
	int					fds[2];
	EventLoop			eventLoop;
	ClientManager		clients;
	ClientIo			io(8);
	RuntimeLoop			runtime(eventLoop, clients, io);

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0)
		throw std::runtime_error("socketpair failed");
	set_non_blocking(fds[0]);
	clients.connections().push_back(ClientConnection(fds[0], 10));
	eventLoop.registerClient(clients.connections().back());
	write(fds[1], "GET", 3);

	RuntimeLoopStats stats = runtime.runCycles(2, 100);

	clients.closeAll(eventLoop);
	close_fd(fds[1]);
	assert_true(stats.cycles == 2, "expected two poll cycles to run");
	assert_true(stats.clientEvents >= 1, "expected at least one client event");
	assert_true(stats.bytesRead == 3, "expected request bytes to be read");
	assert_true(clients.connections().empty(),
		"expected test cleanup to remove clients");
}

static void	test_closed_client_event_marks_client_closing(void)
{
	int					fds[2];
	EventLoop			eventLoop;
	ClientManager		clients;
	ClientIo			io;
	RuntimeLoop			runtime(eventLoop, clients, io);

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0)
		throw std::runtime_error("socketpair failed");
	set_non_blocking(fds[0]);
	clients.connections().push_back(ClientConnection(fds[0], 10));
	eventLoop.registerClient(clients.connections().back());
	close_fd(fds[1]);

	RuntimeLoopStats stats = runtime.runCycle(100);

	assert_true(stats.clientEvents == 1,
		"expected closed peer to produce one client event");
	assert_true(stats.removedClients == 1,
		"expected closing client to be removed after poll closed event");
	assert_true(clients.connections().empty(),
		"expected closed client event to mark and remove client");
}

static void	test_cleanup_closes_timed_out_clients(void)
{
	int					fds[2];
	EventLoop			eventLoop;
	ClientManager		clients;
	ClientIo			io;
	RuntimeLoop			runtime(eventLoop, clients, io);
	std::time_t			now;

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0)
		throw std::runtime_error("socketpair failed");
	clients.connections().push_back(ClientConnection(fds[0], 10));
	eventLoop.registerClient(clients.connections().back());
	now = clients.connections().back().lastActivity() + 10;

	RuntimeLoopStats stats = runtime.cleanup(now, 5);

	close_fd(fds[1]);
	assert_true(stats.removedClients == 1,
		"expected cleanup to report timed-out client removal");
	assert_true(clients.connections().empty(),
		"expected cleanup to remove timed-out client");
	assert_true(eventLoop.watchedCount() == 0,
		"expected cleanup to unregister timed-out client fd");
}

int	main(void)
{
	try
	{
		test_stats_default_values_are_zero();
		test_stats_add_accumulates_values();
		test_one_cycle_handles_ready_read_and_write_clients();
		test_run_cycles_repeats_poll_and_cleanup();
		test_closed_client_event_marks_client_closing();
		test_cleanup_closes_timed_out_clients();
	}
	catch (const std::exception &error)
	{
		std::cerr << "test_runtime_loop: " << error.what() << std::endl;
		return (1);
	}
	std::cout << "test_runtime_loop: OK" << std::endl;
	return (0);
}
