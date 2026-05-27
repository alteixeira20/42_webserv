#include "ClientConnection.hpp"
#include "EventLoop.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>

static void	assert_true(bool condition, const std::string &message)
{
	if (!condition)
		throw std::runtime_error(message);
}

static void	close_pair(int fds[2])
{
	close(fds[0]);
	close(fds[1]);
}

static void	test_reports_ready_listener()
{
	int fds[2];
	EventLoop loop;

	if (pipe(fds) != 0)
		throw std::runtime_error("pipe failed");
	loop.registerListener(fds[0]);
	write(fds[1], "x", 1);

	std::vector<EventLoopEvent> events = loop.pollOnce(100);

	close_pair(fds);
	assert_true(events.size() == 1, "expected one ready listener event");
	assert_true(events[0].fd == fds[0], "expected event fd to match listener fd");
	assert_true(events[0].kind == EventLoopEvent::LISTENER,
		"expected listener event kind");
	assert_true(events[0].readable, "expected listener to be readable");
	assert_true(!events[0].writable, "listener should not be watched for writes");
}

static void	test_client_interest_follows_connection_state()
{
	int readPipe[2];
	int writePipe[2];
	EventLoop loop;
	ClientConnection readingClient(50, 10);
	ClientConnection writingClient(51, 10);

	if (pipe(readPipe) != 0 || pipe(writePipe) != 0)
		throw std::runtime_error("pipe failed");
	readingClient = ClientConnection(readPipe[0], 10);
	writingClient = ClientConnection(writePipe[1], 10);
	writingClient.appendWriteData("HTTP/1.1 200 OK\r\n\r\n");
	writingClient.setState(ClientConnection::WRITING_RESPONSE);
	loop.registerClient(readingClient);
	loop.registerClient(writingClient);
	write(readPipe[1], "G", 1);

	std::vector<EventLoopEvent> events = loop.pollOnce(100);

	close_pair(readPipe);
	close_pair(writePipe);
	assert_true(events.size() == 2,
		"expected one readable client and one writable client event");
	assert_true(events[0].kind == EventLoopEvent::CLIENT
			&& events[1].kind == EventLoopEvent::CLIENT,
		"expected client event kinds");
	assert_true((events[0].readable || events[1].readable),
		"expected one client readable event");
	assert_true((events[0].writable || events[1].writable),
		"expected one client writable event");
}

static void	test_unregistered_fd_is_not_reported()
{
	int fds[2];
	EventLoop loop;

	if (pipe(fds) != 0)
		throw std::runtime_error("pipe failed");
	loop.registerListener(fds[0]);
	loop.unregisterFd(fds[0]);
	write(fds[1], "x", 1);

	std::vector<EventLoopEvent> events = loop.pollOnce(0);

	close_pair(fds);
	assert_true(events.empty(), "expected unregistered fd not to be polled");
}

int	main(void)
{
	try
	{
		test_reports_ready_listener();
		test_client_interest_follows_connection_state();
		test_unregistered_fd_is_not_reported();
	}
	catch (const std::exception &error)
	{
		std::cerr << "test_event_loop: " << error.what() << std::endl;
		return (1);
	}
	std::cout << "test_event_loop: OK" << std::endl;
	return (0);
}
