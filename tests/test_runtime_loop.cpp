#include "RuntimeLoop.hpp"

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

static void	close_pair(int fds[2])
{
	close(fds[0]);
	close(fds[1]);
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

static void	test_one_cycle_does_one_ready_read_and_write_pass()
{
	int					fds[2];
	EventLoop			eventLoop;
	ClientManager		clients;
	ClientIo			io(3);
	RuntimeLoop			runtime(eventLoop, clients, io);

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0)
		throw std::runtime_error("socketpair failed");
	set_non_blocking(fds[0]);
	set_non_blocking(fds[1]);
	clients.connections().push_back(ClientConnection(fds[0], 10));
	clients.connections().back().appendWriteData("abcdef");
	eventLoop.registerClient(clients.connections().back());
	write(fds[1], "123456", 6);

	RuntimeLoopStats first = runtime.runCycle(100);
	std::string firstWrite = read_available(fds[1]);
	RuntimeLoopStats second = runtime.runCycle(100);
	std::string secondWrite = read_available(fds[1]);

	close_pair(fds);
	assert_true(first.clientEvents == 1, "expected one client event in first cycle");
	assert_true(first.bytesRead == 3, "expected one read chunk in first cycle");
	assert_true(first.bytesWritten == 3, "expected one write chunk in first cycle");
	assert_true(firstWrite == "abc", "expected first write chunk only");
	assert_true(second.bytesRead == 3, "expected remaining read chunk in second cycle");
	assert_true(second.bytesWritten == 3, "expected remaining write chunk in second cycle");
	assert_true(secondWrite == "def", "expected second write chunk only");
	assert_true(clients.connections().back().readBuffer() == "123456",
		"expected read buffer to preserve both chunks");
}

static void	test_run_cycles_repeats_poll_and_cleanup()
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

	close_pair(fds);
	assert_true(stats.cycles == 2, "expected two poll cycles to run");
	assert_true(stats.clientEvents >= 1, "expected at least one client event");
	assert_true(stats.bytesRead == 3, "expected request bytes to be read");
	assert_true(clients.connections().size() == 1,
		"expected active connection to remain");
}

int	main(void)
{
	try
	{
		test_one_cycle_does_one_ready_read_and_write_pass();
		test_run_cycles_repeats_poll_and_cleanup();
	}
	catch (const std::exception &error)
	{
		std::cerr << "test_runtime_loop: " << error.what() << std::endl;
		return (1);
	}
	std::cout << "test_runtime_loop: OK" << std::endl;
	return (0);
}
