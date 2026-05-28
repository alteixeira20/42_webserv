#include "runtime/DummyResponseRuntime.hpp"
#include "runtime/ListenerManager.hpp"

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

static void	read_available(int fd, std::string &response)
{
	char	buffer[128];
	ssize_t	count;

	while (true)
	{
		count = read(fd, buffer, sizeof(buffer));
		if (count > 0)
			response.append(buffer, count);
		else
			return ;
	}
}

static void	test_returns_fixed_response_through_runtime_buffers()
{
	ListenerManager				listeners;
	EventLoop					loop;
	ClientManager				clients;
	DummyResponseRuntime		runtime;
	std::vector<ListenConfig>	configs;
	int							clientFd;
	std::string					response;

	configs.push_back(ListenConfig("127.0.0.1", 0));
	listeners.openAll(configs);
	loop.registerListener(listeners.listeners()[0].fd);
	clientFd = connect_to_port(listeners.listeners()[0].boundPort);
	write(clientFd, "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n", 35);

	for (int i = 0; i < 10 && response.find("Hello, world!") == std::string::npos; ++i)
	{
		runtime.pumpOnce(loop, clients, 100);
		read_available(clientFd, response);
	}

	close_fd(clientFd);
	assert_true(response.find("HTTP/1.1 200 OK\r\n") == 0,
		"expected fixed response status line");
	assert_true(response.find("Content-Length: 13\r\n") != std::string::npos,
		"expected fixed response content length");
	assert_true(response.find("Connection: close\r\n") != std::string::npos,
		"expected fixed response to close connection");
	assert_true(response.find("\r\n\r\nHello, world!") != std::string::npos,
		"expected fixed response body");
	assert_true(clients.connections().empty(),
		"expected response runtime to close completed client");
	assert_true(loop.watchedCount() == 1,
		"expected only listener to remain watched");
}

int	main(void)
{
	try
	{
		test_returns_fixed_response_through_runtime_buffers();
	}
	catch (const std::exception &error)
	{
		std::cerr << "test_dummy_response_runtime: " << error.what() << std::endl;
		return (1);
	}
	std::cout << "test_dummy_response_runtime: OK" << std::endl;
	return (0);
}
