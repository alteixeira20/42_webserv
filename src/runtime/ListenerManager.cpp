#include "runtime/ListenerManager.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static std::string	port_to_string(unsigned short port)
{
	std::ostringstream stream;

	stream << port;
	return (stream.str());
}

static std::string	socket_error(const std::string &prefix)
{
	return (prefix + ": " + std::strerror(errno));
}

static void	close_fd(int fd)
{
	if (fd >= 0)
		close(fd);
}

static void	set_reuse_address(int fd)
{
	int enabled = 1;

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
			reinterpret_cast<const char *>(&enabled), sizeof(enabled)) < 0)
		throw std::runtime_error(socket_error("setsockopt(SO_REUSEADDR) failed"));
}

static void	set_non_blocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);

	if (flags < 0)
		throw std::runtime_error(socket_error("fcntl(F_GETFL) failed"));
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
		throw std::runtime_error(socket_error("fcntl(F_SETFL, O_NONBLOCK) failed"));
}

static addrinfo	*resolve_listener_address(const ListenConfig &config)
{
	addrinfo	hints;
	addrinfo	*result;
	int			status;

	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	result = NULL;
	status = getaddrinfo(config.getHost().empty() ? NULL : config.getHost().c_str(),
			port_to_string(config.getPort()).c_str(), &hints, &result);
	if (status != 0)
		throw std::runtime_error("getaddrinfo failed for " + config.getHost() + ": "
			+ gai_strerror(status));
	return (result);
}

static int	try_open_socket(const ListenConfig &config, const addrinfo *address)
{
	int fd;

	fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
	if (fd < 0)
		throw std::runtime_error(socket_error("socket failed"));
	try
	{
		set_reuse_address(fd);
		if (bind(fd, address->ai_addr, address->ai_addrlen) < 0)
			throw std::runtime_error(socket_error("bind failed for "
					+ config.getHost() + ":" + port_to_string(config.getPort())));
		if (listen(fd, SOMAXCONN) < 0)
			throw std::runtime_error(socket_error("listen failed"));
		set_non_blocking(fd);
	}
	catch (...)
	{
		close_fd(fd);
		throw;
	}
	return (fd);
}

static unsigned short	get_bound_port(int fd)
{
	sockaddr_in	address;
	socklen_t	addressLength;

	std::memset(&address, 0, sizeof(address));
	addressLength = sizeof(address);
	if (getsockname(fd, reinterpret_cast<sockaddr *>(&address),
			&addressLength) < 0)
		throw std::runtime_error(socket_error("getsockname failed"));
	return (ntohs(address.sin_port));
}

static ListenerSocket	create_listener_socket(const ListenConfig &config)
{
	addrinfo	*addresses;
	addrinfo	*current;
	int			fd;

	addresses = resolve_listener_address(config);
	fd = -1;
	try
	{
		for (current = addresses; current != NULL; current = current->ai_next)
		{
			try
			{
				fd = try_open_socket(config, current);
				break ;
			}
			catch (const std::runtime_error &)
			{
				if (current->ai_next == NULL)
					throw;
			}
		}
		freeaddrinfo(addresses);
	}
	catch (...)
	{
		freeaddrinfo(addresses);
		throw;
	}
	if (fd < 0)
		throw std::runtime_error("could not open listener socket");
	return (ListenerSocket(fd, config, get_bound_port(fd)));
}

ListenerSocket::ListenerSocket(void) :
	fd(-1),
	config(),
	boundPort(0)
{
}

ListenerSocket::ListenerSocket(int fd, const ListenConfig &config,
	unsigned short boundPort) :
	fd(fd),
	config(config),
	boundPort(boundPort)
{
}

ListenerManager::ListenerManager(void)
{
}

ListenerManager::~ListenerManager(void)
{
	closeAll();
}

void	ListenerManager::openAll(const std::vector<ListenConfig> &configs)
{
	closeAll();
	if (configs.empty())
		throw std::runtime_error("no listener endpoints configured");
	try
	{
		_listeners.reserve(configs.size());
		for (std::size_t i = 0; i < configs.size(); ++i)
			_listeners.push_back(create_listener_socket(configs[i]));
	}
	catch (...)
	{
		closeAll();
		throw;
	}
}

void	ListenerManager::closeAll(void)
{
	for (std::size_t i = 0; i < _listeners.size(); ++i)
		close_fd(_listeners[i].fd);
	_listeners.clear();
}

const std::vector<ListenerSocket>	&ListenerManager::listeners(void) const
{
	return (_listeners);
}
