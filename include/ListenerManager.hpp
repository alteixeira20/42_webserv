#ifndef LISTENER_MANAGER_HPP
# define LISTENER_MANAGER_HPP

# include "ListenerConfig.hpp"

# include <vector>

struct ListenerSocket
{
	int				fd;
	ListenerConfig	config;
	unsigned short	boundPort;

	ListenerSocket(void);
	ListenerSocket(int fd, const ListenerConfig &config, unsigned short boundPort);
};

class ListenerManager
{
public:
	ListenerManager(void);
	~ListenerManager(void);

	void								openAll(const std::vector<ListenerConfig> &configs);
	void								closeAll(void);
	const std::vector<ListenerSocket>	&listeners(void) const;

private:
	std::vector<ListenerSocket>	_listeners;

	ListenerManager(const ListenerManager &other);
	ListenerManager	&operator=(const ListenerManager &other);
};

#endif
