#ifndef DUMMY_RESPONSE_RUNTIME_HPP
# define DUMMY_RESPONSE_RUNTIME_HPP

# include "ClientIo.hpp"
# include "ClientManager.hpp"
# include "EventLoop.hpp"

class DummyResponseRuntime
{
public:
	DummyResponseRuntime(void);

	void	pumpOnce(EventLoop &eventLoop, ClientManager &clients, int timeoutMs);

private:
	ClientIo	_io;

	void	handleClientEvent(const EventLoopEvent &event);
	void	queueResponse(ClientConnection &client) const;
};

#endif
