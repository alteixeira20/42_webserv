#include "DummyResponseRuntime.hpp"

static const char	*fixed_response(void)
{
	return ("HTTP/1.1 200 OK\r\n"
		"Content-Length: 13\r\n"
		"Connection: close\r\n"
		"\r\n"
		"Hello, world!");
}

DummyResponseRuntime::DummyResponseRuntime(void) :
	_io()
{
}

void	DummyResponseRuntime::pumpOnce(EventLoop &eventLoop,
	ClientManager &clients, int timeoutMs)
{
	std::vector<EventLoopEvent> events;

	events = eventLoop.pollOnce(timeoutMs);
	for (std::size_t i = 0; i < events.size(); ++i)
	{
		if (events[i].kind == EventLoopEvent::LISTENER)
			clients.acceptReady(events[i], eventLoop);
		else if (events[i].kind == EventLoopEvent::CLIENT)
			handleClientEvent(events[i]);
	}
	clients.removeClosing(eventLoop);
}

void	DummyResponseRuntime::handleClientEvent(const EventLoopEvent &event)
{
	ClientIoResult	result;

	if (event.error || event.closed)
	{
		event.client->closeWithReason("client poll error");
		return ;
	}
	result = _io.handleReadable(event);
	if (result.bytes > 0 && event.client->state() != ClientConnection::CLOSING)
		queueResponse(*event.client);
	result = _io.handleWritable(event);
	if (result.bytes > 0 && event.client->writeBuffer().empty())
		event.client->closeWithReason("response sent");
}

void	DummyResponseRuntime::queueResponse(ClientConnection &client) const
{
	if (client.state() == ClientConnection::WRITING_RESPONSE)
		return ;
	client.appendWriteData(fixed_response());
	client.setState(ClientConnection::WRITING_RESPONSE);
}
