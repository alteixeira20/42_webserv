#include "ServerRuntime.hpp"

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

static void	test_start_registers_listeners_and_runtime_accepts_clients()
{
	ServerRuntime				runtime;
	std::vector<ListenerConfig>	configs;
	int							clientFd;

	configs.push_back(ListenerConfig("127.0.0.1", 0));
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

static void	test_shutdown_unregisters_and_closes_clients_and_listeners()
{
	ServerRuntime				runtime;
	std::vector<ListenerConfig>	configs;
	int							clientFd;
	int							listenerFd;
	int							acceptedFd;

	configs.push_back(ListenerConfig("127.0.0.1", 0));
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

int	main(void)
{
	try
	{
		test_start_registers_listeners_and_runtime_accepts_clients();
		test_shutdown_unregisters_and_closes_clients_and_listeners();
	}
	catch (const std::exception &error)
	{
		std::cerr << "test_server_runtime: " << error.what() << std::endl;
		return (1);
	}
	std::cout << "test_server_runtime: OK" << std::endl;
	return (0);
}
