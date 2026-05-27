#ifndef LISTENER_CONFIG_HPP
# define LISTENER_CONFIG_HPP

# include <string>

struct ListenerConfig
{
	std::string	host;
	unsigned short	port;

	ListenerConfig(void);
	ListenerConfig(const std::string &host, unsigned short port);
};

#endif
