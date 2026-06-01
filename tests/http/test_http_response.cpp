#include "http/HttpResponse.hpp"
#include "http/ResponseBuilder.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

static void	assert_true(bool condition, const std::string &message)
{
	if (!condition)
		throw std::runtime_error(message);
}

static void	test_serializes_status_headers_and_body(void)
{
	HttpResponse	response;

	response.setStatus(201, "Created");
	response.setHeader("Content-Type", "text/plain");
	response.setBody("hello");
	response.setCloseConnection(true);
	assert_true(response.serialize()
		== "HTTP/1.1 201 Created\r\n"
		"Connection: close\r\n"
		"Content-Length: 5\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"hello",
		"response should serialize status line, headers, and body");
}

static void	test_content_length_tracks_body_size(void)
{
	HttpResponse	response;

	response.setStatus(200, "OK");
	response.setBody("abcdef");
	assert_true(response.serialize().find("Content-Length: 6\r\n")
		!= std::string::npos,
		"response should derive Content-Length from body size");
}

static void	test_content_length_overrides_manual_header(void)
{
	HttpResponse	response;
	std::string		serialized;

	response.setBody("abc");
	response.setHeader("Content-Length", "999");
	serialized = response.serialize();
	assert_true(serialized.find("Content-Length: 3\r\n")
		!= std::string::npos,
		"response should compute Content-Length from body");
	assert_true(serialized.find("Content-Length: 999\r\n")
		== std::string::npos,
		"response should not trust stale Content-Length headers");
}

static void	test_keep_alive_connection_header(void)
{
	HttpResponse	response;
	std::string		serialized;

	response.setCloseConnection(false);
	serialized = response.serialize();
	assert_true(serialized.find("Connection: keep-alive\r\n")
		!= std::string::npos,
		"response should serialize keep-alive when close is false");
}

static void	test_default_reason_phrase(void)
{
	HttpResponse	response;

	response.setStatus(404);
	response.setBody("missing");
	assert_true(response.serialize().find("HTTP/1.1 404 Not Found\r\n") == 0,
		"response should supply default reason phrase");
}

static void	test_reason_phrases_cover_required_statuses(void)
{
	HttpResponse	response;

	response.setStatus(301);
	assert_true(response.serialize().find("HTTP/1.1 301 Moved Permanently\r\n")
		== 0, "301 reason phrase should be known");
	response.setStatus(403);
	assert_true(response.serialize().find("HTTP/1.1 403 Forbidden\r\n") == 0,
		"403 reason phrase should be known");
	response.setStatus(413);
	assert_true(response.serialize().find("HTTP/1.1 413 Payload Too Large\r\n")
		== 0, "413 reason phrase should be known");
}

static void	test_builder_creates_simple_close_response(void)
{
	HttpRequest		request;
	ResponseBuilder	builder;
	HttpResponse	response;
	std::string		serialized;

	request.setMethod(HTTP_METHOD_GET);
	request.setTarget("/hello");
	request.setVersion("HTTP/1.1");
	response = builder.buildSimpleResponse(request);
	serialized = response.serialize();
	assert_true(serialized.find("HTTP/1.1 200 OK\r\n") == 0,
		"builder should create 200 OK");
	assert_true(serialized.find("Content-Type: text/plain\r\n")
		!= std::string::npos,
		"builder should set text/plain");
	assert_true(serialized.find("Connection: close\r\n") != std::string::npos,
		"builder should close connections for now");
	assert_true(serialized.find("webserv response\n") != std::string::npos,
		"builder should include minimal body");
	assert_true(serialized.find("target: /hello\n") != std::string::npos,
		"builder should include request target in body");
}

static void	test_builder_creates_default_error_page(void)
{
	ResponseBuilder	builder;
	HttpResponse	response;
	std::string		serialized;

	response = builder.buildErrorResponse(404);
	serialized = response.serialize();
	assert_true(serialized.find("HTTP/1.1 404 Not Found\r\n") == 0,
		"builder should keep the error status line");
	assert_true(serialized.find("Content-Type: text/html\r\n")
		!= std::string::npos,
		"default error response should be html");
	assert_true(serialized.find("<title>404 Not Found</title>")
		!= std::string::npos,
		"default error page should include title");
	assert_true(serialized.find("<h1>404 Not Found</h1>")
		!= std::string::npos,
		"default error page should include heading");
}

static void	test_builder_creates_custom_error_page(void)
{
	ResponseBuilder	builder;
	HttpResponse	response;
	std::string		serialized;

	response = builder.buildCustomErrorResponse(500,
			"<html><body>custom 500</body></html>", "text/html");
	serialized = response.serialize();
	assert_true(serialized.find("HTTP/1.1 500 Internal Server Error\r\n")
		== 0, "custom error response should preserve status");
	assert_true(serialized.find("Content-Type: text/html\r\n")
		!= std::string::npos,
		"custom error response should preserve content type");
	assert_true(serialized.find("custom 500") != std::string::npos,
		"custom error response should include provided body");
}

int	main(void)
{
	try
	{
		test_serializes_status_headers_and_body();
		test_content_length_tracks_body_size();
		test_content_length_overrides_manual_header();
		test_keep_alive_connection_header();
		test_default_reason_phrase();
		test_reason_phrases_cover_required_statuses();
		test_builder_creates_simple_close_response();
		test_builder_creates_default_error_page();
		test_builder_creates_custom_error_page();
	}
	catch (const std::exception &error)
	{
		std::cerr << "test_http_response: " << error.what() << std::endl;
		return (1);
	}
	std::cout << "test_http_response: OK" << std::endl;
	return (0);
}
