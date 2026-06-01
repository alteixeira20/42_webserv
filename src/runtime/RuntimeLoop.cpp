#include "runtime/RuntimeLoop.hpp"

#include "http/HttpResponse.hpp"
#include "http/ResponseBuilder.hpp"

RuntimeLoopStats::RuntimeLoopStats(void) :
	cycles(0),
	listenerEvents(0),
	clientEvents(0),
	acceptedClients(0),
	bytesRead(0),
	bytesWritten(0),
	removedClients(0)
{
}

void	RuntimeLoopStats::add(const RuntimeLoopStats &other)
{
	cycles += other.cycles;
	listenerEvents += other.listenerEvents;
	clientEvents += other.clientEvents;
	acceptedClients += other.acceptedClients;
	bytesRead += other.bytesRead;
	bytesWritten += other.bytesWritten;
	removedClients += other.removedClients;
}

RuntimeLoop::RuntimeLoop(EventLoop &eventLoop, ClientManager &clients,
	const ClientIo &io) :
	_eventLoop(eventLoop),
	_clients(clients),
	_io(io)
{
}

RuntimeLoopStats	RuntimeLoop::runCycle(int timeoutMs)
{
	std::vector<EventLoopEvent>	events;
	RuntimeLoopStats			stats;

	events = _eventLoop.pollOnce(timeoutMs);
	stats.cycles = 1;
	for (std::size_t i = 0; i < events.size(); ++i)
		handleEvent(events[i], stats);
	queueProcessingResponses();
	stats.removedClients += _clients.removeClosing(_eventLoop);
	return (stats);
}

RuntimeLoopStats	RuntimeLoop::runCycles(std::size_t cycleCount, int timeoutMs)
{
	RuntimeLoopStats	total;

	for (std::size_t i = 0; i < cycleCount; ++i)
		total.add(runCycle(timeoutMs));
	return (total);
}

RuntimeLoopStats	RuntimeLoop::cleanup(std::time_t now,
	std::time_t timeoutSeconds)
{
	RuntimeLoopStats	stats;

	stats.removedClients += _clients.closeTimedOut(_eventLoop, now,
			timeoutSeconds);
	stats.removedClients += _clients.removeClosing(_eventLoop);
	return (stats);
}

void	RuntimeLoop::handleEvent(const EventLoopEvent &event,
	RuntimeLoopStats &stats)
{
	if (event.kind == EventLoopEvent::LISTENER)
	{
		++stats.listenerEvents;
		stats.acceptedClients += _clients.acceptReady(event, _eventLoop);
	}
	else if (event.kind == EventLoopEvent::CLIENT)
		handleClientEvent(event, stats);
}

void	RuntimeLoop::handleClientEvent(const EventLoopEvent &event,
	RuntimeLoopStats &stats)
{
	ClientIoResult	result;

	++stats.clientEvents;
	if (event.client == NULL)
		return ;
	if (event.error || event.closed)
	{
		event.client->closeWithReason("client poll error");
		return ;
	}
	result = _io.handleReadable(event);
	stats.bytesRead += result.bytes;
	result = _io.handleWritable(event);
	stats.bytesWritten += result.bytes;
	closeIfResponseComplete(*event.client);
}

void	RuntimeLoop::queueProcessingResponses(void)
{
	ResponseBuilder	builder;

	for (ClientManager::ConnectionList::iterator it =
		_clients.connections().begin(); it != _clients.connections().end();
		++it)
	{
		if (it->state() == ClientConnection::PROCESSING
			&& it->hasParsedRequest())
		{
			HttpResponse	response = builder.buildSimpleResponse(
					it->getParsedRequest());

			it->appendWriteData(response.serialize());
			it->setState(ClientConnection::WRITING_RESPONSE);
		}
	}
}

void	RuntimeLoop::closeIfResponseComplete(ClientConnection &client)
{
	if (client.state() == ClientConnection::WRITING_RESPONSE
		&& client.writeBuffer().empty())
		client.closeWithReason("response complete");
}
