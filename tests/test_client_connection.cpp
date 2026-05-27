#include "ClientConnection.hpp"

#include <ctime>
#include <iostream>
#include <stdexcept>
#include <string>

static void	assert_true(bool condition, const std::string &message)
{
	if (!condition)
		throw std::runtime_error(message);
}

static void	test_initial_state_tracks_fd_ownership()
{
	ClientConnection connection(42, 7);

	assert_true(connection.fd() == 42, "expected client fd to be stored");
	assert_true(connection.listenerFd() == 7, "expected listener fd to be stored");
	assert_true(connection.state() == ClientConnection::READING_HEADERS,
		"expected new clients to start reading headers");
	assert_true(connection.readBuffer().empty(), "expected empty read buffer");
	assert_true(connection.writeBuffer().empty(), "expected empty write buffer");
	assert_true(connection.closeReason().empty(), "expected empty close reason");
	assert_true(connection.wantsRead(), "expected new clients to want reads");
	assert_true(!connection.wantsWrite(), "expected new clients not to want writes");
	assert_true(connection.lastActivity() <= std::time(NULL),
		"expected activity timestamp to be initialized");
}

static void	test_buffers_and_poll_intent_follow_state()
{
	ClientConnection connection(42, 7);

	connection.appendReadData("GET /");
	assert_true(connection.readBuffer() == "GET /",
		"expected read data to be appended");
	connection.setState(ClientConnection::PROCESSING);
	assert_true(!connection.wantsRead(), "processing clients should not want reads");
	assert_true(!connection.wantsWrite(), "processing clients without data should not want writes");

	connection.appendWriteData("HTTP/1.1 200 OK\r\n\r\n");
	connection.setState(ClientConnection::WRITING_RESPONSE);
	assert_true(!connection.wantsRead(), "writing clients should not want reads");
	assert_true(connection.wantsWrite(), "writing clients with data should want writes");

	connection.consumeWrittenBytes(9);
	assert_true(connection.writeBuffer() == "200 OK\r\n\r\n",
		"expected written bytes to be consumed from the write buffer");
}

static void	test_closing_records_reason_and_disables_io()
{
	ClientConnection connection(42, 7);

	connection.appendWriteData("body");
	connection.closeWithReason("client timeout");
	assert_true(connection.state() == ClientConnection::CLOSING,
		"expected closeWithReason to enter CLOSING");
	assert_true(connection.closeReason() == "client timeout",
		"expected close reason to be stored");
	assert_true(!connection.wantsRead(), "closing clients should not want reads");
	assert_true(!connection.wantsWrite(), "closing clients should not want writes");
}

int	main(void)
{
	try
	{
		test_initial_state_tracks_fd_ownership();
		test_buffers_and_poll_intent_follow_state();
		test_closing_records_reason_and_disables_io();
	}
	catch (const std::exception &error)
	{
		std::cerr << "test_client_connection: " << error.what() << std::endl;
		return (1);
	}
	std::cout << "test_client_connection: OK" << std::endl;
	return (0);
}
