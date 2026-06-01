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

static void	test_default_reason_phrase(void)
{
	HttpResponse	response;

	response.setStatus(404);
	response.setBody("missing");
	assert_true(response.serialize().find("HTTP/1.1 404 Not Found\r\n") == 0,
		"response should supply default reason phrase");
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

int	main(void)
{
	try
	{
		test_serializes_status_headers_and_body();
		test_content_length_tracks_body_size();
		test_default_reason_phrase();
		test_builder_creates_simple_close_response();
	}
	catch (const std::exception &error)
	{
		std::cerr << "test_http_response: " << error.what() << std::endl;
		return (1);
	}
	std::cout << "test_http_response: OK" << std::endl;
	return (0);
}
