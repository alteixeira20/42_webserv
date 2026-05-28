#include "config/Config.hpp"
#include "config/ConfigException.hpp"
#include "config/ConfigParser.hpp"
#include "config/ConfigParserErrors.hpp"
#include "config/ConfigResolver.hpp"
#include "config/ConfigToken.hpp"
#include "config/ConfigTokenizer.hpp"
#include "config/ConfigValidationErrors.hpp"
#include "config/ListenConfig.hpp"
#include "config/RouteConfig.hpp"
#include "config/ServerConfig.hpp"
#include "http/HttpMethod.hpp"

#include <cstddef>
#include <exception>
#include <iostream>
#include <map>
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

static void	assert_size(std::size_t actual, std::size_t expected,
	const std::string &message)
{
	if (actual != expected)
		throw std::runtime_error(message);
}

static Config	parse_config(const std::string &content)
{
	ConfigParser	parser;

	return (parser.parseString(content));
}

static void	expect_config_error(const std::string &content,
	const std::string &message)
{
	try
	{
		parse_config(content);
	}
	catch (const ConfigException &error)
	{
		assert_string(error.what(), message, "config error message");
		return ;
	}
	throw std::runtime_error("expected config parse failure: " + message);
}

static void	expect_file_error(const std::string &path,
	const std::string &message)
{
	ConfigParser	parser;

	try
	{
		parser.parseFile(path);
	}
	catch (const ConfigException &error)
	{
		assert_string(error.what(), message, "file error message");
		return ;
	}
	throw std::runtime_error("expected config file failure: " + message);
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
	assert_true(server.getListens().size() == 3,
		"duplicate listens should be preserved for validation");
	assert_true(server.getServerNames().size() == 2,
		"duplicate server names should be ignored in one server object");
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

static void	test_parser_listen_values(void)
{
	Config config;
	const ServerConfig *server;

	config = parse_config("server { listen 8080; }\n");
	server = &config.getServers()[0];
	assert_size(config.getServers().size(), 1, "one server should parse");
	assert_size(server->getListens().size(), 1, "one listen should parse");
	assert_string(server->getListens()[0].getHost(), "0.0.0.0",
		"port-only listen should use wildcard host");
	assert_uint(server->getListens()[0].getPort(), 8080,
		"listen port should parse");
	config = parse_config("server { listen 127.0.0.1:9090; }\n");
	server = &config.getServers()[0];
	assert_string(server->getListens()[0].getHost(), "127.0.0.1",
		"host:port listen should parse host");
	assert_uint(server->getListens()[0].getPort(), 9090,
		"host:port listen should parse port");
}

static void	test_parser_server_directives(void)
{
	ConfigParser parser;
	Config config = parser.parseFile("tests/config/fixtures/valid_full.conf");
	const ServerConfig &server = config.getServers()[0];
	ServerConfig::ErrorPageMap errors = server.getErrorPages();

	assert_size(config.getServers().size(), 1, "fixture should parse one server");
	assert_size(server.getServerNames().size(), 2,
		"server_name should parse multiple names");
	assert_string(server.getServerNames()[0], "localhost",
		"first server_name should match");
	assert_string(server.getRoot(), "www", "server root should parse");
	assert_string(server.getIndex(), "home.html", "server index should parse");
	assert_true(server.getClientMaxBodySize() == 2048,
		"client_max_body_size should parse");
	assert_string(errors.find(404)->second, "www/errors/4xx.html",
		"multi-status error_page should map 404");
	assert_string(errors.find(403)->second, "www/errors/4xx.html",
		"multi-status error_page should map 403");
	assert_string(errors.find(500)->second, "www/errors/5xx.html",
		"single error_page should map 500");
}

static void	test_parser_route_directives(void)
{
	ConfigParser parser;
	Config config = parser.parseFile("tests/config/fixtures/valid_full.conf");
	const ServerConfig &server = config.getServers()[0];
	const RouteConfig &root = server.getRoutes()[0];
	const RouteConfig &upload = server.getRoutes()[1];
	const RouteConfig &old = server.getRoutes()[2];
	const RouteConfig &cgi = server.getRoutes()[3];

	assert_size(server.getRoutes().size(), 4, "location blocks should parse");
	assert_string(root.getPath(), "/", "root route path should parse");
	assert_true(root.hasRoot(), "route root should be present");
	assert_string(root.getRoot(), "www/root", "route root should parse");
	assert_true(root.hasIndex(), "route index should be present");
	assert_string(root.getIndex(), "index.html", "route index should parse");
	assert_true(root.hasAutoIndex(), "route autoindex should be present");
	assert_true(!root.getAutoIndex(), "route autoindex off should parse");
	assert_size(root.getAllowedMethods().size(), 2,
		"allowed_methods should parse and deduplicate");
	assert_true(containsHttpMethod(root.getAllowedMethods(), HTTP_METHOD_GET),
		"allowed_methods should contain GET");
	assert_string(upload.getUploadDir(), "uploads", "upload_dir should parse");
	assert_true(upload.getAutoIndex(), "autoindex on should parse");
	assert_uint(old.getRedirectStatus(), 301, "redirect status should parse");
	assert_string(old.getRedirectTarget(), "/", "redirect target should parse");
	assert_string(cgi.getCgiMap().find(".py")->second, "/usr/bin/python3",
		"cgi directive should parse");
}

static void	test_parser_rejections(void)
{
	expect_config_error("", ConfigValidationErrors::EMPTY_CONFIG);
	expect_config_error("server { root www; }",
		ConfigValidationErrors::SERVER_WITHOUT_LISTEN);
	expect_config_error("server { listen :8080; }",
		ConfigParserErrors::INVALID_LISTEN_VALUE);
	expect_config_error("server { listen 0; }",
		ConfigParserErrors::INVALID_LISTEN_PORT);
	expect_config_error("server { listen 65536; }",
		ConfigParserErrors::INVALID_LISTEN_PORT);
	expect_config_error("server { listen abc; }",
		ConfigParserErrors::INVALID_LISTEN_PORT);
	expect_config_error("server { listen 8080; client_max_body_size 0; }",
		ConfigParserErrors::INVALID_BODY_SIZE);
}

static void	test_route_rejections(void)
{
	expect_config_error(
		"server { listen 8080; location / { allowed_methods PATCH; } }",
		ConfigParserErrors::INVALID_ALLOWED_METHOD);
	expect_config_error(
		"server { listen 8080; location / { autoindex maybe; } }",
		ConfigParserErrors::INVALID_BOOLEAN_VALUE);
	expect_config_error(
		"server { listen 8080; location /old { redirect 200 /; } }",
		ConfigParserErrors::INVALID_REDIRECT_STATUS);
	expect_config_error(
		"server { listen 8080; location /a {} location /a {} }",
		ConfigValidationErrors::DUPLICATE_ROUTE_PATH);
	expect_config_error("server { listen 8080; location img {} }",
		ConfigValidationErrors::INVALID_ROUTE_PATH);
}

static void	test_virtual_host_validation(void)
{
	parse_config("server { listen 8080; server_name one.test; }\n"
		"server { listen 8080; server_name two.test; }\n");
	expect_config_error("server { listen 8080; listen 8080; }",
		ConfigValidationErrors::DUPLICATE_LISTEN_IN_SERVER);
	expect_config_error("server { listen 8080; }\nserver { listen 8080; }\n",
		ConfigValidationErrors::DUPLICATE_DEFAULT_SERVER);
	expect_config_error(
		"server { listen 8080; server_name same.test; }\n"
		"server { listen 8080; server_name same.test; }\n",
		ConfigValidationErrors::DUPLICATE_SERVER_NAME);
}

static void	test_config_parser_files(void)
{
	ConfigParser parser;
	Config config = parser.parseFile("tests/config/fixtures/valid_minimal.conf");

	assert_size(config.getServers().size(), 1,
		"parseFile should return semantic server output");
	parser.parseFile("tests/config/fixtures/valid_comments.conf");
	expect_file_error("tests/config/fixtures/does_not_exist.conf",
		ConfigParserErrors::COULD_NOT_OPEN_FILE);
	try
	{
		parser.parseFile("tests/config/fixtures/invalid_missing_semicolon.conf");
	}
	catch (const ConfigException &error)
	{
		assert_string(error.what(), ConfigParserErrors::EXPECTED_DIRECTIVE_SEMICOLON,
			"missing semicolon fixture should fail");
		return ;
	}
	throw std::runtime_error("invalid_missing_semicolon should fail");
}

static void	test_config_resolver(void)
{
	Config config = parse_config(
		"server { listen 127.0.0.1:8080; location / {} "
		"location /img {} location /img/icons {} }\n"
		"server { listen 127.0.0.1:8080; server_name named.test; "
		"location /named {} }\n");
	ConfigResolver resolver;
	ListenConfig endpoint("127.0.0.1", 8080);
	const ServerConfig *defaultServer;
	const ServerConfig *namedServer;
	const RouteConfig *route;

	defaultServer = resolver.findServer(config, endpoint, "unknown.test");
	namedServer = resolver.findServer(config, endpoint, "named.test");
	assert_true(defaultServer == &config.getServers()[0],
		"first server on endpoint should be default");
	assert_true(namedServer == &config.getServers()[1],
		"server_name match should win");
	route = resolver.findRoute(*defaultServer, "/img/icons/logo.png");
	assert_string(route->getPath(), "/img/icons",
		"longest route prefix should win");
	route = resolver.findRoute(*defaultServer, "/img2/logo.png");
	assert_string(route->getPath(), "/",
		"route prefix should respect segment boundary");
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
		test_parser_listen_values();
		test_parser_server_directives();
		test_parser_route_directives();
		test_parser_rejections();
		test_route_rejections();
		test_virtual_host_validation();
		test_config_parser_files();
		test_config_resolver();
	}
	catch (const std::exception &error)
	{
		std::cerr << "test_config_foundation: " << error.what() << std::endl;
		return (1);
	}
	std::cout << "test_config_foundation: OK" << std::endl;
	return (0);
}
