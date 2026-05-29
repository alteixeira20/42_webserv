#ifndef RUNTIME_LOOP_HPP
# define RUNTIME_LOOP_HPP

# include "runtime/ClientIo.hpp"
# include "runtime/ClientManager.hpp"
# include "runtime/EventLoop.hpp"

# include <cstddef>
# include <ctime>

struct RuntimeLoopStats
{
	std::size_t	cycles;
	std::size_t	listenerEvents;
	std::size_t	clientEvents;
	std::size_t	acceptedClients;
	std::size_t	bytesRead;
	std::size_t	bytesWritten;
	std::size_t	removedClients;

	RuntimeLoopStats(void);
	void	add(const RuntimeLoopStats &other);
};

class RuntimeLoop
{
	public:
		RuntimeLoop(EventLoop &eventLoop, ClientManager &clients,
			const ClientIo &io);

		RuntimeLoopStats	runCycle(int timeoutMs);
		RuntimeLoopStats	runCycles(std::size_t cycleCount, int timeoutMs);
		RuntimeLoopStats	cleanup(std::time_t now, std::time_t timeoutSeconds);

	private:
		EventLoop		&_eventLoop;
		ClientManager	&_clients;
		const ClientIo	&_io;

		RuntimeLoop(const RuntimeLoop &other);
		RuntimeLoop	&operator=(const RuntimeLoop &other);

		void	handleEvent(const EventLoopEvent &event, RuntimeLoopStats &stats);
		void	handleClientEvent(const EventLoopEvent &event,
			RuntimeLoopStats &stats);
};

#endif
