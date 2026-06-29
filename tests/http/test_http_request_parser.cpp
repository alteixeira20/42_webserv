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
	assert_true(parser.getRequest().bodySize() == 0,
		"request without content length should have empty body");
}

static void	test_content_length_zero_completes_after_headers()
{
	HttpRequestParser	parser;

	parser.append("POST /submit HTTP/1.1\r\n"
		"Host: localhost\r\nContent-Length: 0\r\n\r\n");
	assert_true(parser.isComplete(),
		"zero content length should complete after headers");
	assert_true(parser.getRequest().bodySize() == 0,
		"zero content length should store an empty body");
}

static void	test_content_length_body_same_append()
{
	HttpRequestParser	parser;

	parser.append("POST /submit HTTP/1.1\r\n"
		"Host: localhost\r\nContent-Length: 5\r\n\r\nhello");
	assert_true(parser.isComplete(),
		"body in same append should complete request");
	assert_true(parser.getRequest().getBody() == "hello",
		"body should be stored from same append");
	assert_true(parser.getRequest().bodySize() == 5,
		"body size should match content length");
}

static void	test_content_length_body_split_across_appends()
{
	HttpRequestParser	parser;

	parser.append("POST /submit HTTP/1.1\r\n"
		"Host: localhost\r\nContent-Length: 11\r\n\r\nhello");
	assert_true(!parser.isComplete(),
		"partial body should keep parser incomplete");
	assert_true(parser.getState() == HttpRequestParser::PARSING_BODY,
		"parser should wait in body state");
	parser.append(" world");
	assert_true(parser.isComplete(),
		"split body should complete after all bytes arrive");
	assert_true(parser.getRequest().getBody() == "hello world",
		"split body should be combined");
}

static void	test_extra_bytes_after_content_length_are_not_body()
{
	HttpRequestParser	parser;

	parser.append("POST /submit HTTP/1.1\r\n"
		"Content-Length: 5\r\n\r\nhelloextra");
	assert_true(parser.isComplete(),
		"parser should complete after content length bytes");
	assert_true(parser.getRequest().getBody() == "hello",
		"body should include exactly content length bytes");
	assert_true(parser.getRequest().bodySize() == 5,
		"body size should match exact content length");
}

static void	test_invalid_content_length_fails()
{
	HttpRequestParser	parser;

	parser.append("POST /submit HTTP/1.1\r\n"
		"Content-Length: nope\r\n\r\n");
	assert_true(parser.hasError(),
		"invalid content length should fail");
	assert_true(parser.getErrorStatus() == 400,
		"invalid content length should be 400");
}

static void	test_negative_content_length_fails()
{
	HttpRequestParser	parser;

	parser.append("POST /submit HTTP/1.1\r\n"
		"Content-Length: -1\r\n\r\n");
	assert_true(parser.hasError(),
		"negative content length should fail");
	assert_true(parser.getErrorStatus() == 400,
		"negative content length should be 400");
}

static void	test_incomplete_content_length_body_waits()
{
	HttpRequestParser	parser;

	parser.append("POST /submit HTTP/1.1\r\n"
		"Content-Length: 8\r\n\r\nbody");
	assert_true(!parser.hasError(),
		"incomplete body should not fail");
	assert_true(!parser.isComplete(),
		"incomplete body should not complete");
	assert_true(parser.getState() == HttpRequestParser::PARSING_BODY,
		"incomplete body should keep parser in body state");
}

static void	test_conflicting_content_length_fails()
{
	HttpRequestParser	parser;

	parser.append("POST /submit HTTP/1.1\r\n"
		"Content-Length: 4\r\ncontent-length: 5\r\n\r\nhello");
	assert_true(parser.hasError(),
		"conflicting content length headers should fail");
	assert_true(parser.getErrorStatus() == 400,
		"conflicting content length should be 400");
}

static void	test_overflow_content_length_fails()
{
	HttpRequestParser	parser;

	parser.append("POST /submit HTTP/1.1\r\n"
		"Content-Length: 999999999999999999999999999999\r\n\r\n");
	assert_true(parser.hasError(),
		"overflow content length should fail");
	assert_true(parser.getErrorStatus() == 400,
		"overflow content length should be 400");
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

static void	test_chunked_single_chunk_is_decoded()
{
	HttpRequestParser	parser;

	parser.append("POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Transfer-Encoding: chunked\r\n\r\n"
		"5\r\nhello\r\n"
		"0\r\n\r\n");
	assert_true(parser.isComplete(),
		"single chunked chunk should complete request");
	assert_true(parser.getRequest().getBody() == "hello",
		"single chunk body should be stored");
	assert_true(parser.getRequest().bodySize() == 5,
		"body size should match decoded chunk length");
}

static void	test_chunked_multiple_chunks_are_concatenated()
{
	HttpRequestParser	parser;

	parser.append("POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Transfer-Encoding: chunked\r\n\r\n"
		"5\r\nhello\r\n"
		"6\r\n world\r\n"
		"0\r\n\r\n");
	assert_true(parser.isComplete(),
		"multiple chunks should complete request");
	assert_true(parser.getRequest().getBody() == "hello world",
		"multiple chunks should be concatenated");
}

static void	test_chunked_split_across_appends()
{
	HttpRequestParser	parser;

	parser.append("POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Transfer-Encoding: chunked\r\n\r\n");
	assert_true(!parser.isComplete(),
		"request with chunked body should not complete after headers");
	assert_true(parser.getState() == HttpRequestParser::PARSING_CHUNKED_BODY,
		"parser should enter chunked body state");
	parser.append("5\r\nhel");
	assert_true(!parser.isComplete(),
		"partial chunk data should not complete request");
	parser.append("lo\r\n0\r\n\r\n");
	assert_true(parser.isComplete(),
		"chunked request should complete after last chunk");
	assert_true(parser.getRequest().getBody() == "hello",
		"split chunk should produce correct body");
}

static void	test_chunked_hex_size_is_parsed_case_insensitively()
{
	HttpRequestParser	parser;

	parser.append("POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Transfer-Encoding: chunked\r\n\r\n"
		"A\r\n0123456789\r\n"
		"0\r\n\r\n");
	assert_true(parser.isComplete(),
		"uppercase hex chunk size should be accepted");
	assert_true(parser.getRequest().bodySize() == 10,
		"hex chunk size A should decode to 10 bytes");
}

static void	test_chunked_chunk_extension_is_ignored()
{
	HttpRequestParser	parser;

	parser.append("POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Transfer-Encoding: chunked\r\n\r\n"
		"5;ext=value\r\nhello\r\n"
		"0\r\n\r\n");
	assert_true(parser.isComplete(),
		"chunk extension should not prevent parsing");
	assert_true(parser.getRequest().getBody() == "hello",
		"chunk extension should be ignored");
}

static void	test_chunked_malformed_size_fails()
{
	HttpRequestParser	parser;

	parser.append("POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Transfer-Encoding: chunked\r\n\r\n"
		"xyz\r\ndata\r\n"
		"0\r\n\r\n");
	assert_true(parser.hasError(),
		"malformed chunk size should fail");
	assert_true(parser.getErrorStatus() == 400,
		"malformed chunk size should produce 400");
}

static void	test_chunked_missing_crlf_after_data_fails()
{
	HttpRequestParser	parser;

	parser.append("POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Transfer-Encoding: chunked\r\n\r\n"
		"5\r\nhelloXX"
		"0\r\n\r\n");
	assert_true(parser.hasError(),
		"missing CRLF after chunk data should fail");
	assert_true(parser.getErrorStatus() == 400,
		"missing chunk CRLF should produce 400");
}

static void	test_chunked_and_content_length_conflict_fails()
{
	HttpRequestParser	parser;

	parser.append("POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Transfer-Encoding: chunked\r\n"
		"Content-Length: 5\r\n\r\n");
	assert_true(parser.hasError(),
		"both Transfer-Encoding and Content-Length should fail");
	assert_true(parser.getErrorStatus() == 400,
		"conflicting framing should produce 400");
}

static void	test_chunked_empty_body_zero_chunk_only()
{
	HttpRequestParser	parser;

	parser.append("POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Transfer-Encoding: chunked\r\n\r\n"
		"0\r\n\r\n");
	assert_true(parser.isComplete(),
		"zero-chunk only should complete request");
	assert_true(parser.getRequest().getBody().empty(),
		"zero-chunk only should produce empty body");
}

static void	test_chunked_trailer_headers_are_ignored()
{
	HttpRequestParser	parser;

	parser.append("POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Transfer-Encoding: chunked\r\n\r\n"
		"5\r\nhello\r\n"
		"0\r\n"
		"Trailer-Field: value\r\n"
		"\r\n");
	assert_true(parser.isComplete(),
		"trailer headers after last chunk should be consumed");
	assert_true(parser.getRequest().getBody() == "hello",
		"body should not include trailer header content");
}

static void	test_reset_clears_parser_state()
{
	HttpRequestParser	parser;

	parser.append("POST /old HTTP/1.1\r\nHost: localhost\r\n"
		"Content-Length: 4\r\n\r\nbody");
	assert_true(parser.getRequest().getBody() == "body",
		"first request should store body");
	assert_true(parser.isComplete(), "first request should complete");
	parser.reset();
	assert_true(parser.getState() == HttpRequestParser::PARSING_REQUEST_LINE,
		"reset should return parser to request line state");
	assert_true(parser.getErrorStatus() == 0, "reset should clear error status");
	assert_true(parser.getRequest().getTarget().empty(),
		"reset should clear request target");
	assert_true(!parser.getRequest().hasHeader("host"),
		"reset should clear request headers");
	assert_true(parser.getRequest().getBody().empty(),
		"reset should clear request body");
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
		test_content_length_zero_completes_after_headers();
		test_content_length_body_same_append();
		test_content_length_body_split_across_appends();
		test_extra_bytes_after_content_length_are_not_body();
		test_invalid_content_length_fails();
		test_negative_content_length_fails();
		test_incomplete_content_length_body_waits();
		test_conflicting_content_length_fails();
		test_overflow_content_length_fails();
		test_trims_header_whitespace();
		test_headers_can_arrive_in_chunks();
		test_rejects_bad_request_line();
		test_rejects_unknown_method();
		test_rejects_invalid_target();
		test_rejects_unsupported_version();
		test_rejects_malformed_header();
		test_rejects_empty_header_name();
		test_rejects_oversized_header_section();
		test_chunked_single_chunk_is_decoded();
		test_chunked_multiple_chunks_are_concatenated();
		test_chunked_split_across_appends();
		test_chunked_hex_size_is_parsed_case_insensitively();
		test_chunked_chunk_extension_is_ignored();
		test_chunked_malformed_size_fails();
		test_chunked_missing_crlf_after_data_fails();
		test_chunked_and_content_length_conflict_fails();
		test_chunked_empty_body_zero_chunk_only();
		test_chunked_trailer_headers_are_ignored();
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
