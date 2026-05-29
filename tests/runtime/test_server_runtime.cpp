#include "runtime/ServerRuntime.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

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

static int	connect_to_port(unsigned short port)
{
	int			fd;
	sockaddr_in	address;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		throw std::runtime_error("socket failed");
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	address.sin_port = htons(port);
	if (connect(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0)
		throw std::runtime_error("connect failed");
	set_non_blocking(fd);
	return (fd);
}

static void	test_before_start_runtime_methods_are_noops(void)
{
	ServerRuntime runtime;

	assert_stats_zero(runtime.runCycle(0),
		"expected runCycle before start to be a no-op");
	assert_stats_zero(runtime.runCycles(3, 0),
		"expected runCycles before start to be a no-op");
	assert_stats_zero(runtime.cleanup(std::time(NULL), 1),
		"expected cleanup before start to be a no-op");
	assert_true(runtime.listenerCount() == 0,
		"expected no listeners before start");
	assert_true(runtime.clientCount() == 0, "expected no clients before start");
	assert_true(runtime.watchedCount() == 0,
		"expected no watched fds before start");
}

static void	test_start_registers_listeners_and_runtime_accepts_clients(void)
{
	ServerRuntime				runtime;
	std::vector<ListenConfig>	configs;
	int							clientFd;

	configs.push_back(ListenConfig("127.0.0.1", 0));
	runtime.start(configs);
	assert_true(runtime.listenerCount() == 1,
		"expected runtime to open one listener");
	assert_true(runtime.watchedCount() == 1,
		"expected runtime to register listener in event loop");

	clientFd = connect_to_port(runtime.listenerPort(0));
	RuntimeLoopStats acceptStats = runtime.runCycle(100);
	write(clientFd, "GET", 3);
	RuntimeLoopStats readStats = runtime.runCycle(100);

	close_fd(clientFd);
	assert_true(acceptStats.acceptedClients == 1,
		"expected runtime loop to accept connected client");
	assert_true(runtime.clientCount() == 1,
		"expected accepted client to be tracked");
	assert_true(readStats.bytesRead == 3,
		"expected runtime loop to process guarded client read");
	assert_true(runtime.watchedCount() == 2,
		"expected listener plus client to be watched");
}

static void	test_shutdown_unregisters_and_closes_clients_and_listeners(void)
{
	ServerRuntime				runtime;
	std::vector<ListenConfig>	configs;
	int							clientFd;
	int							listenerFd;
	int							acceptedFd;

	configs.push_back(ListenConfig("127.0.0.1", 0));
	runtime.start(configs);
	listenerFd = runtime.listenerFd(0);
	clientFd = connect_to_port(runtime.listenerPort(0));
	runtime.runCycle(100);
	acceptedFd = runtime.clients().connections().front().fd();

	runtime.shutdown();

	close_fd(clientFd);
	assert_true(runtime.listenerCount() == 0,
		"expected shutdown to clear listeners");
	assert_true(runtime.clientCount() == 0,
		"expected shutdown to clear clients");
	assert_true(runtime.watchedCount() == 0,
		"expected shutdown to unregister all fds");
	assert_true(fcntl(listenerFd, F_GETFL, 0) < 0,
		"expected listener fd to be closed");
	assert_true(fcntl(acceptedFd, F_GETFL, 0) < 0,
		"expected accepted client fd to be closed");
}

static void	test_listener_fd_out_of_range_throws(void)
{
	ServerRuntime runtime;
	bool		  threw;

	threw = false;
	try
	{
		runtime.listenerFd(0);
	}
	catch (const std::out_of_range &)
	{
		threw = true;
	}
	assert_true(threw, "expected listenerFd out of range to throw");
}

static void	test_listener_port_out_of_range_throws(void)
{
	ServerRuntime runtime;
	bool		  threw;

	threw = false;
	try
	{
		runtime.listenerPort(0);
	}
	catch (const std::out_of_range &)
	{
		threw = true;
	}
	assert_true(threw, "expected listenerPort out of range to throw");
}

static void	test_shutdown_may_be_called_twice(void)
{
	ServerRuntime				runtime;
	std::vector<ListenConfig>	configs;

	configs.push_back(ListenConfig("127.0.0.1", 0));
	runtime.start(configs);
	runtime.shutdown();
	runtime.shutdown();
	assert_true(runtime.listenerCount() == 0,
		"expected double shutdown to leave no listeners");
	assert_true(runtime.clientCount() == 0,
		"expected double shutdown to leave no clients");
	assert_true(runtime.watchedCount() == 0,
		"expected double shutdown to leave no watched fds");
}

int	main(void)
{
	try
	{
		test_before_start_runtime_methods_are_noops();
		test_start_registers_listeners_and_runtime_accepts_clients();
		test_shutdown_unregisters_and_closes_clients_and_listeners();
		test_listener_fd_out_of_range_throws();
		test_listener_port_out_of_range_throws();
		test_shutdown_may_be_called_twice();
	}
	catch (const std::exception &error)
	{
		std::cerr << "test_server_runtime: " << error.what() << std::endl;
		return (1);
	}
	std::cout << "test_server_runtime: OK" << std::endl;
	return (0);
}
