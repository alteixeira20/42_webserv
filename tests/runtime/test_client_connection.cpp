#include "runtime/ClientConnection.hpp"

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

static void	test_complete_get_request_is_parsed()
{
	ClientConnection	connection(42, 7);

	connection.appendReadData("GET /index.html HTTP/1.1\r\n"
		"Host: localhost\r\n\r\n");
	assert_true(connection.readBuffer() == "GET /index.html HTTP/1.1\r\n"
		"Host: localhost\r\n\r\n",
		"expected raw read buffer to remain available");
	assert_true(connection.hasParsedRequest(),
		"expected complete request to be visible");
	assert_true(connection.state() == ClientConnection::PROCESSING,
		"expected complete headers to move connection to processing");
	assert_true(connection.getParsedRequest().getMethod() == HTTP_METHOD_GET,
		"expected parsed request method");
	assert_true(connection.getParsedRequest().getTarget() == "/index.html",
		"expected parsed request target");
	assert_true(connection.getParsedRequest().getHeader("host") == "localhost",
		"expected parsed host header");
}

static void	test_split_request_is_parsed_across_appends()
{
	ClientConnection	connection(42, 7);

	connection.appendReadData("GET /split HTTP/1.1\r\nHo");
	assert_true(!connection.hasParsedRequest(),
		"expected partial request to remain incomplete");
	assert_true(connection.state() == ClientConnection::READING_HEADERS,
		"expected partial request to keep reading headers");
	connection.appendReadData("st: localhost\r\n\r\n");
	assert_true(connection.hasParsedRequest(),
		"expected split request to complete after second append");
	assert_true(connection.getParsedRequest().getTarget() == "/split",
		"expected parser to combine split request chunks");
	assert_true(connection.state() == ClientConnection::PROCESSING,
		"expected split complete headers to move connection to processing");
}

static void	test_post_body_keeps_connection_reading_until_complete()
{
	ClientConnection	connection(42, 7);

	connection.appendReadData("POST /submit HTTP/1.1\r\n"
		"Host: localhost\r\nContent-Length: 11\r\n\r\nhello");
	assert_true(!connection.hasParsedRequest(),
		"partial body should not expose a complete request");
	assert_true(connection.state() == ClientConnection::READING_BODY,
		"connection should keep reading while body is incomplete");
	assert_true(connection.wantsRead(),
		"connection should still want reads for incomplete body");
	connection.appendReadData(" world");
	assert_true(connection.hasParsedRequest(),
		"complete body should expose parsed request");
	assert_true(connection.state() == ClientConnection::PROCESSING,
		"complete body should move connection to processing");
	assert_true(connection.getParsedRequest().getBody() == "hello world",
		"parsed request body should be accessible");
}

static void	test_invalid_content_length_closes_connection()
{
	ClientConnection	connection(42, 7);

	connection.appendReadData("POST /submit HTTP/1.1\r\n"
		"Content-Length: nope\r\n\r\n");
	assert_true(connection.hasParserError(),
		"invalid content length should expose parser error");
	assert_true(connection.parserErrorStatus() == 400,
		"invalid content length status should be visible");
	assert_true(connection.state() == ClientConnection::CLOSING,
		"invalid content length should close connection");
	assert_true(connection.closeReason() == "http request parse error",
		"invalid content length should use parser close reason");
}

static void	test_parser_completion_is_exposed()
{
	ClientConnection	connection(42, 7);

	connection.appendReadData("GET /visible HTTP/1.1\r\n\r\n");
	assert_true(connection.hasParsedRequest(),
		"expected parsed request flag");
	assert_true(connection.getRequestParser().isComplete(),
		"expected parser completion to be available");
	assert_true(connection.getParsedRequest().getVersion() == "HTTP/1.1",
		"expected parsed request fields to be available");
}

static void	test_parser_error_closes_connection()
{
	ClientConnection	connection(42, 7);

	connection.appendReadData("GET missing-slash HTTP/1.1\r\n");
	assert_true(connection.hasParserError(),
		"expected parser error to be visible");
	assert_true(connection.parserErrorStatus() == 400,
		"expected parser error status to be visible");
	assert_true(connection.state() == ClientConnection::CLOSING,
		"expected parser error to close the connection");
	assert_true(connection.closeReason() == "http request parse error",
		"expected parser error close reason");
	assert_true(!connection.wantsRead(),
		"expected parser error connection not to want reads");
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

static void	test_chunked_body_keeps_connection_reading_until_complete()
{
	ClientConnection	connection(42, 7);

	connection.appendReadData("POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\nTransfer-Encoding: chunked\r\n\r\n");
	assert_true(!connection.hasParsedRequest(),
		"chunked request with no chunks yet should not be complete");
	assert_true(connection.state() == ClientConnection::READING_BODY,
		"connection should keep reading for chunked body");
	assert_true(connection.wantsRead(),
		"connection should want reads while awaiting chunk data");
	connection.appendReadData("5\r\nhello\r\n0\r\n\r\n");
	assert_true(connection.hasParsedRequest(),
		"chunked request should complete after last chunk");
	assert_true(connection.state() == ClientConnection::PROCESSING,
		"complete chunked request should move to processing");
	assert_true(connection.getParsedRequest().getBody() == "hello",
		"chunked body should be decoded and stored");
}

static void	test_timeout_uses_last_activity()
{
	ClientConnection	connection(42, 7);
	std::time_t		lastActivity;

	lastActivity = connection.lastActivity();
	assert_true(!connection.isTimedOut(lastActivity - 1, 0),
		"future activity timestamps should not time out");
	assert_true(connection.isTimedOut(lastActivity, 0),
		"zero timeout should expire at last activity time");
	assert_true(!connection.isTimedOut(lastActivity, 10),
		"timeout should wait for full timeout interval");
	assert_true(connection.isTimedOut(lastActivity + 10, 10),
		"timeout should expire after full timeout interval");
}

int	main()
{
	try
	{
		test_initial_state_tracks_fd_ownership();
		test_buffers_and_poll_intent_follow_state();
		test_complete_get_request_is_parsed();
		test_split_request_is_parsed_across_appends();
		test_post_body_keeps_connection_reading_until_complete();
		test_invalid_content_length_closes_connection();
		test_chunked_body_keeps_connection_reading_until_complete();
		test_parser_completion_is_exposed();
		test_parser_error_closes_connection();
		test_closing_records_reason_and_disables_io();
		test_timeout_uses_last_activity();
	}
	catch (const std::exception &error)
	{
		std::cerr << "test_client_connection: " << error.what() << std::endl;
		return (1);
	}
	std::cout << "test_client_connection: OK" << std::endl;
	return (0);
}
