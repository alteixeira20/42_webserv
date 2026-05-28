#ifndef CLIENT_MANAGER_HPP
# define CLIENT_MANAGER_HPP

# include "ClientConnection.hpp"
# include "EventLoop.hpp"

# include <ctime>
# include <list>
# include <string>

class ClientManager
{
public:
	typedef std::list<ClientConnection>	ConnectionList;

	ClientManager(void);
	~ClientManager(void);

	std::size_t				acceptReady(const EventLoopEvent &event,
								EventLoop &eventLoop);
	std::size_t				closeTimedOut(EventLoop &eventLoop,
								std::time_t now,
								std::time_t timeoutSeconds);
	std::size_t				removeClosing(EventLoop &eventLoop);
	void					closeAll(void);
	void					closeAll(EventLoop &eventLoop);
	ConnectionList			&connections(void);
	const ConnectionList		&connections(void) const;

private:
	ConnectionList	_connections;

	ClientManager(const ClientManager &other);
	ClientManager	&operator=(const ClientManager &other);

	void	addClient(int clientFd, int listenerFd, EventLoop &eventLoop);
	ConnectionList::iterator	removeClient(ConnectionList::iterator it,
								EventLoop &eventLoop);
};

#endif
