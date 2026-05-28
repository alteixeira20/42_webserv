#include "runtime/ListenerManager.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <vector>

static void	assert_true(bool condition, const std::string &message)
{
	if (!condition)
		throw std::runtime_error(message);
}

static bool	is_non_blocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	return (flags >= 0 && (flags & O_NONBLOCK));
}

static unsigned short	socket_bound_port(int fd)
{
	sockaddr_in	address;
	socklen_t	addressLength;

	addressLength = sizeof(address);
	if (getsockname(fd, reinterpret_cast<sockaddr *>(&address),
			&addressLength) < 0)
		throw std::runtime_error("getsockname failed");
	return (ntohs(address.sin_port));
}

static void	test_opens_all_configured_endpoints()
{
	std::vector<ListenConfig> endpoints;
	endpoints.push_back(ListenConfig("127.0.0.1", 0));
	endpoints.push_back(ListenConfig("127.0.0.1", 0));

	ListenerManager manager;
	manager.openAll(endpoints);

	assert_true(manager.listeners().size() == endpoints.size(),
		"expected one listener socket per configured endpoint");
	for (std::size_t i = 0; i < manager.listeners().size(); ++i)
	{
		assert_true(manager.listeners()[i].fd >= 0,
			"expected listener fd to be valid");
		assert_true(is_non_blocking(manager.listeners()[i].fd),
			"expected listener fd to be non-blocking");
		assert_true(manager.listeners()[i].config.getHost() == endpoints[i].getHost(),
			"expected listener to keep its host config");
		assert_true(manager.listeners()[i].config.getPort() == endpoints[i].getPort(),
			"expected listener to keep its port config");
		assert_true(manager.listeners()[i].boundPort > 0,
			"expected port 0 listener to expose assigned bound port");
		assert_true(manager.listeners()[i].boundPort
			== socket_bound_port(manager.listeners()[i].fd),
			"expected listener boundPort to match getsockname");
	}
	std::vector<int> fds;
	for (std::size_t i = 0; i < manager.listeners().size(); ++i)
		fds.push_back(manager.listeners()[i].fd);
	manager.closeAll();
	assert_true(manager.listeners().empty(), "expected closeAll to clear listeners");
	for (std::size_t i = 0; i < fds.size(); ++i)
		assert_true(fcntl(fds[i], F_GETFL, 0) < 0,
			"expected closeAll to close listener fd");
}

static void	test_rejects_empty_endpoint_list()
{
	ListenerManager manager;
	std::vector<ListenConfig> endpoints;

	try
	{
		manager.openAll(endpoints);
	}
	catch (const std::runtime_error &)
	{
		return;
	}
	throw std::runtime_error("expected empty endpoint list to be rejected");
}

int	main(void)
{
	try
	{
		test_opens_all_configured_endpoints();
		test_rejects_empty_endpoint_list();
	}
	catch (const std::exception &error)
	{
		std::cerr << "test_listener_manager: " << error.what() << std::endl;
		return (1);
	}
	std::cout << "test_listener_manager: OK" << std::endl;
	return (0);
}
