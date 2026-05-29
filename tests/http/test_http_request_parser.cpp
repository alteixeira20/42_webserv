#include "http/HttpRequestParser.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

static void	assert_true(bool condition, const std::string &message)
{
	if (!condition)
		throw std::runtime_error(message);
}

static void	test_parses_complete_request_line()
{
	HttpRequestParser	parser;

	parser.append("GET /index.html HTTP/1.1\r\n");
	assert_true(!parser.hasError(), "parser should not fail valid request line");
	assert_true(parser.getState() == HttpRequestParser::PARSING_HEADERS,
		"parser should move to headers after request line");
	assert_true(parser.getRequest().getMethod() == HTTP_METHOD_GET,
		"method should be GET");
	assert_true(parser.getRequest().getTarget() == "/index.html",
		"target should be parsed");
	assert_true(parser.getRequest().getVersion() == "HTTP/1.1",
		"version should be parsed");
}

static void	test_waits_for_full_line()
{
	HttpRequestParser	parser;

	parser.append("GET /ind");
	assert_true(!parser.hasError(), "partial line should not fail");
	assert_true(parser.getState() == HttpRequestParser::PARSING_REQUEST_LINE,
		"parser should still wait for request line");
	parser.append("ex.html HTTP/1.1\r\n");
	assert_true(parser.getState() == HttpRequestParser::PARSING_HEADERS,
		"parser should parse after CRLF arrives");
	assert_true(parser.getRequest().getTarget() == "/index.html",
		"target should include both chunks");
}

static void	test_parses_headers_and_completes_request()
{
	HttpRequestParser	parser;

	parser.append("GET / HTTP/1.1\r\nHost: localhost\r\n");
	assert_true(parser.getState() == HttpRequestParser::PARSING_HEADERS,
		"parser should wait for blank line");
	parser.append("User-Agent: test\r\n\r\n");
	assert_true(parser.isComplete(), "blank line should complete request");
	assert_true(parser.getRequest().hasHeader("host"),
		"host header should exist");
	assert_true(parser.getRequest().hasHeader("HOST"),
		"header lookup should be case-insensitive");
	assert_true(parser.getRequest().getHeader("Host") == "localhost",
		"host header value should be parsed");
	assert_true(parser.getRequest().getHeader("user-agent") == "test",
		"user-agent header value should be parsed");
}

static void	test_trims_header_whitespace()
{
	HttpRequestParser	parser;

	parser.append("GET / HTTP/1.1\r\nHost:   localhost  \r\n\r\n");
	assert_true(parser.isComplete(), "request should complete");
	assert_true(parser.getRequest().getHeader("host") == "localhost",
		"header value should be trimmed");
}

static void	test_headers_can_arrive_in_chunks()
{
	HttpRequestParser	parser;

	parser.append("GET / HTTP/1.1\r\nHo");
	parser.append("st: local");
	parser.append("host\r\n\r\n");
	assert_true(parser.isComplete(), "chunked headers should complete");
	assert_true(parser.getRequest().getHeader("host") == "localhost",
		"chunked header should be parsed");
}

static void	test_rejects_bad_request_line()
{
	HttpRequestParser	parser;

	parser.append("GET /missing-version\r\n");
	assert_true(parser.hasError(), "malformed request line should fail");
	assert_true(parser.getErrorStatus() == 400,
		"malformed request line should be 400");
}

static void	test_rejects_unknown_method()
{
	HttpRequestParser	parser;

	parser.append("PATCH / HTTP/1.1\r\n");
	assert_true(parser.hasError(), "unknown method should fail");
	assert_true(parser.getErrorStatus() == 405,
		"unknown method should be 405");
}

static void	test_rejects_invalid_target()
{
	HttpRequestParser	parser;

	parser.append("GET index.html HTTP/1.1\r\n");
	assert_true(parser.hasError(), "target without slash should fail");
	assert_true(parser.getErrorStatus() == 400,
		"invalid target should be 400");
}

static void	test_rejects_unsupported_version()
{
	HttpRequestParser	parser;

	parser.append("GET / HTTP/1.0\r\n");
	assert_true(parser.hasError(), "unsupported version should fail");
	assert_true(parser.getErrorStatus() == 505,
		"unsupported version should be 505");
}

static void	test_rejects_malformed_header()
{
	HttpRequestParser	parser;

	parser.append("GET / HTTP/1.1\r\nBrokenHeader\r\n");
	assert_true(parser.hasError(), "header without colon should fail");
	assert_true(parser.getErrorStatus() == 400,
		"malformed header should be 400");
}

static void	test_rejects_empty_header_name()
{
	HttpRequestParser	parser;

	parser.append("GET / HTTP/1.1\r\n: value\r\n");
	assert_true(parser.hasError(), "empty header name should fail");
	assert_true(parser.getErrorStatus() == 400,
		"empty header name should be 400");
}

static void	test_rejects_oversized_header_section()
{
	HttpRequestParser	parser;
	std::string			large;

	large.assign(9000, 'A');
	parser.append("GET / HTTP/1.1\r\n");
	parser.append(large);
	assert_true(parser.hasError(), "oversized header section should fail");
	assert_true(parser.getErrorStatus() == 431,
		"oversized header section should be 431");
}

static void	test_reset_clears_parser_state()
{
	HttpRequestParser	parser;

	parser.append("GET /old HTTP/1.1\r\nHost: localhost\r\n\r\n");
	assert_true(parser.isComplete(), "first request should complete");
	parser.reset();
	assert_true(parser.getState() == HttpRequestParser::PARSING_REQUEST_LINE,
		"reset should return parser to request line state");
	assert_true(parser.getErrorStatus() == 0, "reset should clear error status");
	assert_true(parser.getRequest().getTarget().empty(),
		"reset should clear request target");
	assert_true(!parser.getRequest().hasHeader("host"),
		"reset should clear request headers");
	parser.append("GET / HTTP/1.0\r\n");
	assert_true(parser.hasError(), "parser should fail second request");
	parser.reset();
	parser.append("GET /ok HTTP/1.1\r\n\r\n");
	assert_true(parser.isComplete(), "parser should parse after reset");
	assert_true(parser.getRequest().getTarget() == "/ok",
		"reset parser should store new request");
}

int	main()
{
	try
	{
		test_parses_complete_request_line();
		test_waits_for_full_line();
		test_parses_headers_and_completes_request();
		test_trims_header_whitespace();
		test_headers_can_arrive_in_chunks();
		test_rejects_bad_request_line();
		test_rejects_unknown_method();
		test_rejects_invalid_target();
		test_rejects_unsupported_version();
		test_rejects_malformed_header();
		test_rejects_empty_header_name();
		test_rejects_oversized_header_section();
		test_reset_clears_parser_state();
	}
	catch (const std::exception &error)
	{
		std::cerr << "test_http_request_parser: "
			<< error.what() << std::endl;
		return (1);
	}
	std::cout << "test_http_request_parser: OK" << std::endl;
	return (0);
}
