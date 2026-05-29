#include "http/HttpRequestParser.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

static void	assert_true(bool condition, const std::string &message)
{
	if (!condition)
		throw std::runtime_error(message);
}

static void	test_parses_complete_request_line(void)
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

static void	test_waits_for_full_line(void)
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

static void	test_rejects_bad_request_line(void)
{
	HttpRequestParser	parser;

	parser.append("GET /missing-version\r\n");
	assert_true(parser.hasError(), "malformed request line should fail");
	assert_true(parser.getErrorStatus() == 400,
		"malformed request line should be 400");
}

static void	test_rejects_unknown_method(void)
{
	HttpRequestParser	parser;

	parser.append("PATCH / HTTP/1.1\r\n");
	assert_true(parser.hasError(), "unknown method should fail");
	assert_true(parser.getErrorStatus() == 405,
		"unknown method should be 405");
}

static void	test_rejects_invalid_target(void)
{
	HttpRequestParser	parser;

	parser.append("GET index.html HTTP/1.1\r\n");
	assert_true(parser.hasError(), "target without slash should fail");
	assert_true(parser.getErrorStatus() == 400,
		"invalid target should be 400");
}

static void	test_rejects_unsupported_version(void)
{
	HttpRequestParser	parser;

	parser.append("GET / HTTP/1.0\r\n");
	assert_true(parser.hasError(), "unsupported version should fail");
	assert_true(parser.getErrorStatus() == 505,
		"unsupported version should be 505");
}

int	main(void)
{
	try
	{
		test_parses_complete_request_line();
		test_waits_for_full_line();
		test_rejects_bad_request_line();
		test_rejects_unknown_method();
		test_rejects_invalid_target();
		test_rejects_unsupported_version();
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
