#include "runtime/ClientIo.hpp"

#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
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

static void	set_non_blocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);

	if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl failed");
}

static EventLoopEvent	client_event(ClientConnection &client,
	bool readable, bool writable)
{
	EventLoopEvent event;

	event.fd = client.fd();
	event.kind = EventLoopEvent::CLIENT;
	event.client = &client;
	event.readable = readable;
	event.writable = writable;
	return (event);
}

static void	test_read_appends_partial_bytes_without_losing_remainder()
{
	int					fds[2];
	ClientIo			io(4);
	ClientConnection	connection(-1, 10);

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0)
		throw std::runtime_error("socketpair failed");
	set_non_blocking(fds[0]);
	connection = ClientConnection(fds[0], 10);
	write(fds[1], "GET /", 5);

	ClientIoResult first = io.handleReadable(client_event(connection, true, false));
	ClientIoResult second = io.handleReadable(client_event(connection, true, false));

	close_pair(fds);
	assert_true(first.bytes == 4, "expected first read to consume chunk size");
	assert_true(second.bytes == 1, "expected second read to consume remainder");
	assert_true(connection.readBuffer() == "GET /",
		"expected read buffer to preserve partial reads in order");
	assert_true(connection.state() == ClientConnection::READING_HEADERS,
		"expected successful read to keep reading state");
}

static void	test_peer_close_moves_connection_to_closing()
{
	int					fds[2];
	ClientIo			io(8);
	ClientConnection	connection(-1, 10);

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0)
		throw std::runtime_error("socketpair failed");
	set_non_blocking(fds[0]);
	connection = ClientConnection(fds[0], 10);
	close(fds[1]);

	ClientIoResult result = io.handleReadable(client_event(connection, true, false));

	close(fds[0]);
	assert_true(result.peerClosed, "expected EOF to be reported");
	assert_true(connection.state() == ClientConnection::CLOSING,
		"expected EOF to close the connection");
	assert_true(connection.closeReason() == "client closed connection",
		"expected EOF close reason");
}

static void	test_write_consumes_only_written_bytes()
{
	int					fds[2];
	char				buffer[8];
	ClientIo			io(3);
	ClientConnection	connection(-1, 10);

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0)
		throw std::runtime_error("socketpair failed");
	set_non_blocking(fds[0]);
	connection = ClientConnection(fds[0], 10);
	connection.appendWriteData("abcdef");
	connection.setState(ClientConnection::WRITING_RESPONSE);

	ClientIoResult result = io.handleWritable(client_event(connection, false, true));
	ssize_t readCount = read(fds[1], buffer, sizeof(buffer));

	close_pair(fds);
	assert_true(result.bytes == 3, "expected write to send one chunk");
	assert_true(readCount == 3, "expected peer to receive written chunk");
	assert_true(std::string(buffer, buffer + readCount) == "abc",
		"expected written bytes to preserve order");
	assert_true(connection.writeBuffer() == "def",
		"expected unsent bytes to remain buffered");
	assert_true(connection.wantsWrite(), "expected connection to still want writes");
}

static void	test_io_ignores_events_without_matching_readiness()
{
	int					fds[2];
	ClientIo			io(4);
	ClientConnection	connection(-1, 10);

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0)
		throw std::runtime_error("socketpair failed");
	set_non_blocking(fds[0]);
	connection = ClientConnection(fds[0], 10);
	write(fds[1], "NOPE", 4);
	connection.appendWriteData("WAIT");
	connection.setState(ClientConnection::WRITING_RESPONSE);

	ClientIoResult readResult = io.handleReadable(client_event(connection, false, false));
	ClientIoResult writeResult = io.handleWritable(client_event(connection, false, false));

	close_pair(fds);
	assert_true(readResult.bytes == 0, "expected unreadable event not to read");
	assert_true(writeResult.bytes == 0, "expected unwritable event not to write");
	assert_true(connection.readBuffer().empty(), "expected read buffer unchanged");
	assert_true(connection.writeBuffer() == "WAIT", "expected write buffer unchanged");
}

static void	test_read_would_block_keeps_connection_open()
{
	int					fds[2];
	ClientIo			io(4);
	ClientConnection	connection(-1, 10);

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0)
		throw std::runtime_error("socketpair failed");
	set_non_blocking(fds[0]);
	connection = ClientConnection(fds[0], 10);

	ClientIoResult result = io.handleReadable(client_event(connection, true, false));

	close_pair(fds);
	assert_true(result.bytes == 0, "expected would-block read to report zero bytes");
	assert_true(!result.peerClosed, "expected would-block read not to report EOF");
	assert_true(connection.state() == ClientConnection::READING_HEADERS,
		"expected would-block read to keep connection open");
	assert_true(connection.closeReason().empty(),
		"expected would-block read not to record close reason");
}

static void	test_read_error_moves_connection_to_closing()
{
	int					fds[2];
	ClientIo			io(4);
	ClientConnection	connection(-1, 10);

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0)
		throw std::runtime_error("socketpair failed");
	connection = ClientConnection(fds[0], 10);
	close(fds[0]);

	ClientIoResult result = io.handleReadable(client_event(connection, true, false));

	close(fds[1]);
	assert_true(result.bytes == 0, "expected read error to report zero bytes");
	assert_true(!result.peerClosed, "expected read error not to report EOF");
	assert_true(connection.state() == ClientConnection::CLOSING,
		"expected read error to close the connection");
	assert_true(connection.closeReason() == "client read error",
		"expected read error close reason");
}

static void	test_write_error_moves_connection_to_closing()
{
	int					fds[2];
	ClientIo			io(4);
	ClientConnection	connection(-1, 10);

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0)
		throw std::runtime_error("socketpair failed");
	connection = ClientConnection(fds[0], 10);
	connection.appendWriteData("body");
	connection.setState(ClientConnection::WRITING_RESPONSE);
	close(fds[0]);

	ClientIoResult result = io.handleWritable(client_event(connection, false, true));

	close(fds[1]);
	assert_true(result.bytes == 0, "expected write error to report zero bytes");
	assert_true(connection.state() == ClientConnection::CLOSING,
		"expected write error to close the connection");
	assert_true(connection.closeReason() == "client write error",
		"expected write error close reason");
}

int	main(void)
{
	try
	{
		test_read_appends_partial_bytes_without_losing_remainder();
		test_peer_close_moves_connection_to_closing();
		test_write_consumes_only_written_bytes();
		test_io_ignores_events_without_matching_readiness();
		test_read_would_block_keeps_connection_open();
		test_read_error_moves_connection_to_closing();
		test_write_error_moves_connection_to_closing();
	}
	catch (const std::exception &error)
	{
		std::cerr << "test_client_io: " << error.what() << std::endl;
		return (1);
	}
	std::cout << "test_client_io: OK" << std::endl;
	return (0);
}
