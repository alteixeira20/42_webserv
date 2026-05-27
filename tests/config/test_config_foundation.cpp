#include "config/Config.hpp"
#include "config/ConfigException.hpp"
#include "config/ConfigParser.hpp"
#include "config/ConfigToken.hpp"
#include "config/ConfigTokenizer.hpp"
#include "config/ListenConfig.hpp"
#include "config/RouteConfig.hpp"
#include "config/ServerConfig.hpp"
#include "http/HttpMethod.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

static void	assert_true(bool condition, const std::string &message)
{
	if (!condition)
		throw std::runtime_error(message);
}

static void	assert_string(const std::string &actual,
	const std::string &expected, const std::string &message)
{
	if (actual != expected)
		throw std::runtime_error(message + " expected '" + expected
			+ "' got '" + actual + "'");
}

static void	assert_uint(unsigned int actual, unsigned int expected,
	const std::string &message)
{
	if (actual != expected)
		throw std::runtime_error(message);
}

static void	test_http_methods(void)
{
	std::vector<HttpMethod> methods;

	assert_true(parseHttpMethod("GET") == HTTP_METHOD_GET, "GET should parse");
	assert_true(parseHttpMethod("POST") == HTTP_METHOD_POST, "POST should parse");
	assert_true(parseHttpMethod("DELETE") == HTTP_METHOD_DELETE,
		"DELETE should parse");
	assert_true(parseHttpMethod("PATCH") == HTTP_METHOD_UNKNOWN,
		"unknown method should stay representable");
	assert_string(httpMethodToStr(HTTP_METHOD_GET), "GET",
		"GET should stringify");
	assert_string(httpMethodToStr(HTTP_METHOD_POST), "POST",
		"POST should stringify");
	assert_string(httpMethodToStr(HTTP_METHOD_DELETE), "DELETE",
		"DELETE should stringify");
	assert_string(httpMethodToStr(HTTP_METHOD_UNKNOWN), "UNKNOWN",
		"unknown method should stringify");
	assert_true(isSupportedHttpMethod(HTTP_METHOD_GET), "GET is supported");
	assert_true(isSupportedHttpMethod(HTTP_METHOD_POST), "POST is supported");
	assert_true(isSupportedHttpMethod(HTTP_METHOD_DELETE), "DELETE is supported");
	assert_true(!isSupportedHttpMethod(HTTP_METHOD_UNKNOWN),
		"UNKNOWN is not supported");
	methods.push_back(HTTP_METHOD_GET);
	methods.push_back(HTTP_METHOD_DELETE);
	assert_true(containsHttpMethod(methods, HTTP_METHOD_GET),
		"vector should contain GET");
	assert_true(!containsHttpMethod(methods, HTTP_METHOD_POST),
		"vector should not contain POST");
}

static void	test_config_token(void)
{
	ConfigToken defaultToken;
	ConfigToken token(CONFIG_TOKEN_WORD, "server", 3, 5);
	ConfigToken copied(token);
	ConfigToken assigned;

	assigned = token;
	assert_true(defaultToken.getType() == CONFIG_TOKEN_END,
		"default token should be END");
	assert_string(token.getValue(), "server", "token value should be stored");
	assert_uint(token.getLine(), 3, "token line should be stored");
	assert_uint(token.getColumn(), 5, "token column should be stored");
	assert_true(copied.getType() == CONFIG_TOKEN_WORD,
		"copy should preserve token type");
	assert_string(assigned.getValue(), "server",
		"assignment should preserve value");
}

static void	test_config_exception(void)
{
	ConfigException error("bad config", 7, 9);
	ConfigException copied(error);
	ConfigException assigned;

	assigned = error;
	assert_string(error.what(), "bad config", "what() should return message");
	assert_uint(error.getLine(), 7, "line should be stored");
	assert_uint(error.getColumn(), 9, "column should be stored");
	assert_string(copied.what(), "bad config", "copy should preserve message");
	assert_uint(assigned.getLine(), 7, "assignment should preserve line");
}

static void	test_config_tokenizer(void)
{
	ConfigTokenizer tokenizer;
	std::vector<ConfigToken> tokens = tokenizer.tokenize(
		"server {\n"
		"  listen 8080; # ignored comment\n"
		"}\n");

	assert_true(tokens.size() == 7, "tokenizer should append END token");
	assert_true(tokens[0].getType() == CONFIG_TOKEN_WORD,
		"first token should be a word");
	assert_string(tokens[0].getValue(), "server", "first word should match");
	assert_uint(tokens[0].getLine(), 1, "server line should be 1");
	assert_uint(tokens[0].getColumn(), 1, "server column should be 1");
	assert_true(tokens[1].getType() == CONFIG_TOKEN_OPEN_BRACE,
		"open brace should be tokenized");
	assert_true(tokens[2].getType() == CONFIG_TOKEN_WORD,
		"listen should be tokenized as word");
	assert_string(tokens[2].getValue(), "listen", "listen word should match");
	assert_uint(tokens[2].getLine(), 2, "listen line should be 2");
	assert_uint(tokens[2].getColumn(), 3, "listen column should be 3");
	assert_string(tokens[3].getValue(), "8080", "port word should match");
	assert_true(tokens[4].getType() == CONFIG_TOKEN_SEMICOLON,
		"semicolon should be tokenized");
	assert_true(tokens[5].getType() == CONFIG_TOKEN_CLOSE_BRACE,
		"close brace should be tokenized after comment");
	assert_true(tokens[6].getType() == CONFIG_TOKEN_END,
		"last token should be END");
}

static void	test_listen_config(void)
{
	ListenConfig defaults;
	ListenConfig first("127.0.0.1", 8080);
	ListenConfig same("127.0.0.1", 8080);
	ListenConfig different("0.0.0.0", 8080);

	assert_string(defaults.getHost(), "0.0.0.0",
		"default listen host should match current code");
	assert_uint(defaults.getPort(), 0,
		"default listen port should match current code");
	assert_string(first.getHost(), "127.0.0.1", "host should be stored");
	assert_uint(first.getPort(), 8080, "port should be stored");
	assert_true(first.equals(same), "same endpoints should be equal");
	assert_true(!first.equals(different),
		"different endpoints should not be equal");
	first.setHost("localhost");
	first.setPort(9090);
	assert_string(first.getHost(), "localhost", "setHost should update host");
	assert_uint(first.getPort(), 9090, "setPort should update port");
}

static void	test_route_config(void)
{
	RouteConfig route;

	assert_true(!route.hasRoot(), "default route should not have root");
	assert_true(!route.hasIndex(), "default route should not have index");
	assert_true(!route.hasAutoIndex(), "default route should not have autoindex");
	assert_true(!route.hasRedirect(), "default route should not have redirect");
	assert_true(!route.hasUploadDir(), "default route should not have upload dir");
	route.setPath("/upload");
	route.setRoot("www");
	route.setIndex("index.html");
	route.setAutoIndex(true);
	route.setRedirect(301, "/new");
	route.setUploadDir("/tmp/uploads");
	route.addAllowedMethod(HTTP_METHOD_GET);
	route.addAllowedMethod(HTTP_METHOD_GET);
	route.addAllowedMethod(HTTP_METHOD_POST);
	route.addCgiHandler(".py", "/usr/bin/python3");
	assert_string(route.getPath(), "/upload", "path should be stored");
	assert_true(route.hasRoot(), "setRoot should activate hasRoot");
	assert_string(route.getRoot(), "www", "root should be stored");
	assert_true(route.hasIndex(), "setIndex should activate hasIndex");
	assert_string(route.getIndex(), "index.html", "index should be stored");
	assert_true(route.hasAutoIndex(), "setAutoIndex should activate flag");
	assert_true(route.getAutoIndex(), "autoindex value should be stored");
	assert_true(route.hasRedirect(), "setRedirect should activate flag");
	assert_uint(route.getRedirectStatus(), 301,
		"redirect status should be stored");
	assert_string(route.getRedirectTarget(), "/new",
		"redirect target should be stored");
	assert_true(route.hasUploadDir(), "setUploadDir should activate flag");
	assert_string(route.getUploadDir(), "/tmp/uploads",
		"upload dir should be stored");
	assert_true(route.getAllowedMethods().size() == 2,
		"duplicate allowed methods should be ignored");
	assert_string(route.getCgiMap().find(".py")->second, "/usr/bin/python3",
		"CGI handler should be stored");
}

static void	test_server_config(void)
{
	ServerConfig server;
	RouteConfig route;

	server.addListen(ListenConfig("127.0.0.1", 8080));
	server.addListen(ListenConfig("127.0.0.1", 8080));
	server.addListen(ListenConfig("0.0.0.0", 8081));
	server.addServerName("localhost");
	server.addServerName("localhost");
	server.addServerName("example.test");
	server.addErrorPage(404, "/errors/404.html");
	route.setPath("/");
	server.addRoute(route);
	assert_true(server.getListens().size() == 2,
		"duplicate listens should be ignored");
	assert_true(server.getServerNames().size() == 2,
		"duplicate server names should be ignored");
	assert_string(server.getIndex(), "index.html",
		"default index should match current code");
	assert_true(server.getClientMaxBodySize() == 1000000,
		"default body size should match current code");
	assert_string(server.getErrorPages().find(404)->second, "/errors/404.html",
		"error page should be stored");
	assert_true(server.getRoutes().size() == 1, "route should be stored");
}

static void	test_config_unique_listens(void)
{
	Config config;
	ServerConfig first;
	ServerConfig second;
	std::vector<ListenConfig> endpoints;

	first.addListen(ListenConfig("127.0.0.1", 8080));
	first.addListen(ListenConfig("127.0.0.1", 8081));
	second.addListen(ListenConfig("127.0.0.1", 8080));
	second.addListen(ListenConfig("0.0.0.0", 8080));
	config.addServer(first);
	config.addServer(second);
	endpoints = config.getUniqueListens();
	assert_true(config.getServers().size() == 2, "servers should be stored");
	assert_true(endpoints.size() == 3,
		"unique listens should deduplicate across servers");
}

static void	test_config_parser_foundation(void)
{
	ConfigParser parser;
	Config config = parser.parseString("server { listen 8080; }");

	assert_true(config.getServers().empty(),
		"semantic parsing is not implemented, config should remain empty");
	config = parser.parseFile("tests/config/fixtures/valid_minimal.conf");
	assert_true(config.getServers().empty(),
		"parseFile should run pipeline without semantic server output yet");
	try
	{
		parser.parseFile("tests/config/fixtures/does_not_exist.conf");
	}
	catch (const ConfigException &error)
	{
		assert_string(error.what(), "could not open config file",
			"missing file should throw ConfigException");
		return ;
	}
	throw std::runtime_error("missing config file should throw ConfigException");
}

int	main(void)
{
	try
	{
		test_http_methods();
		test_config_token();
		test_config_exception();
		test_config_tokenizer();
		test_listen_config();
		test_route_config();
		test_server_config();
		test_config_unique_listens();
		test_config_parser_foundation();
	}
	catch (const std::exception &error)
	{
		std::cerr << "test_config_foundation: " << error.what() << std::endl;
		return (1);
	}
	std::cout << "test_config_foundation: OK" << std::endl;
	return (0);
}
