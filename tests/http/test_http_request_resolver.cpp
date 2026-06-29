#include "config/Config.hpp"
#include "config/ListenConfig.hpp"
#include "config/RouteConfig.hpp"
#include "config/ServerConfig.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpRequestResolver.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

static void	assert_true(bool condition, const std::string &message)
{
	if (!condition)
		throw std::runtime_error(message);
}

static ServerConfig	makeServer(const std::string &listenHost,
	unsigned int listenPort, const std::string &serverName)
{
	ServerConfig	server;

	server.addListen(ListenConfig(listenHost, listenPort));
	if (!serverName.empty())
		server.addServerName(serverName);

	return (server);
}

static void	addRoute(ServerConfig &server, const std::string &path)
{
	RouteConfig	route;

	route.setPath(path);
	server.addRoute(route);
}

static HttpRequest	makeRequest(const std::string &target,
	const std::string &host)
{
	HttpRequest	request;

	request.setMethod(HTTP_METHOD_GET);
	request.setTarget(target);
	request.setVersion("HTTP/1.1");
	if (!host.empty())
		request.setHeader("host", host);

	return (request);
}

static void	test_host_header_selects_matching_server_name()
{
	Config			config;
	ListenConfig	listen("0.0.0.0", 8080);
	ServerConfig	first;
	ServerConfig	second;

	first = makeServer("0.0.0.0", 8080, "first.example.com");
	addRoute(first, "/");
	second = makeServer("0.0.0.0", 8080, "second.example.com");
	addRoute(second, "/");
	config.addServer(first);
	config.addServer(second);

	HttpRequestResolver	resolver;
	HttpRequest			request;
	HttpRequestResolver::Result	result;

	request = makeRequest("/", "second.example.com");
	result = resolver.resolve(config, listen, request);
	assert_true(result.valid, "matched host should produce valid result");
	assert_true(result.server != NULL, "matched host should select a server");
	assert_true(result.server->getServerNames().size() == 1
		&& result.server->getServerNames()[0] == "second.example.com",
		"second.example.com host should select second server");
}

static void	test_no_host_falls_back_to_default_server()
{
	Config			config;
	ListenConfig	listen("0.0.0.0", 8080);
	ServerConfig	first;
	ServerConfig	second;

	first = makeServer("0.0.0.0", 8080, "first.example.com");
	addRoute(first, "/");
	second = makeServer("0.0.0.0", 8080, "second.example.com");
	addRoute(second, "/");
	config.addServer(first);
	config.addServer(second);

	HttpRequestResolver			resolver;
	HttpRequestResolver::Result	result;

	result = resolver.resolve(config, listen, makeRequest("/", ""));
	assert_true(result.valid, "no host should produce valid result");
	assert_true(result.server != NULL, "no host should select default server");
	assert_true(result.server->getServerNames().size() == 1
		&& result.server->getServerNames()[0] == "first.example.com",
		"no host should fall back to first server on that listener");
}

static void	test_unmatched_host_falls_back_to_default_server()
{
	Config			config;
	ListenConfig	listen("0.0.0.0", 8080);
	ServerConfig	server;

	server = makeServer("0.0.0.0", 8080, "known.example.com");
	addRoute(server, "/");
	config.addServer(server);

	HttpRequestResolver			resolver;
	HttpRequestResolver::Result	result;

	result = resolver.resolve(config, listen,
		makeRequest("/", "unknown.example.com"));
	assert_true(result.valid, "unmatched host should produce valid result");
	assert_true(result.server != NULL,
		"unmatched host should fall back to default server");
	assert_true(result.server->getServerNames()[0] == "known.example.com",
		"unmatched host should return the only server for that listener");
}

static void	test_host_with_port_matches_server_name_without_port()
{
	Config			config;
	ListenConfig	listen("0.0.0.0", 8080);
	ServerConfig	server;

	server = makeServer("0.0.0.0", 8080, "example.com");
	addRoute(server, "/");
	config.addServer(server);

	HttpRequestResolver			resolver;
	HttpRequestResolver::Result	result;

	result = resolver.resolve(config, listen,
		makeRequest("/", "example.com:8080"));
	assert_true(result.valid, "host with port should produce valid result");
	assert_true(result.server != NULL,
		"host with port should match server_name without port");
}

static void	test_longest_route_prefix_is_selected()
{
	Config			config;
	ListenConfig	listen("0.0.0.0", 8080);
	ServerConfig	server;

	server = makeServer("0.0.0.0", 8080, "");
	addRoute(server, "/");
	addRoute(server, "/images");
	addRoute(server, "/images/large");
	config.addServer(server);

	HttpRequestResolver			resolver;
	HttpRequestResolver::Result	result;

	result = resolver.resolve(config, listen,
		makeRequest("/images/large/photo.jpg", "localhost"));
	assert_true(result.valid, "deep path should produce valid result");
	assert_true(result.route != NULL,
		"deep path should match a route");
	assert_true(result.route->getPath() == "/images/large",
		"longest prefix should be selected for deep path");
}

static void	test_route_boundary_is_respected()
{
	Config			config;
	ListenConfig	listen("0.0.0.0", 8080);
	ServerConfig	server;

	server = makeServer("0.0.0.0", 8080, "");
	addRoute(server, "/images");
	config.addServer(server);

	HttpRequestResolver			resolver;
	HttpRequestResolver::Result	result;

	result = resolver.resolve(config, listen,
		makeRequest("/images2/photo.jpg", "localhost"));
	assert_true(result.valid, "path with similar prefix should be valid");
	assert_true(result.route == NULL,
		"/images2 should not match route /images");
}

static void	test_root_route_matches_when_nothing_else_does()
{
	Config			config;
	ListenConfig	listen("0.0.0.0", 8080);
	ServerConfig	server;

	server = makeServer("0.0.0.0", 8080, "");
	addRoute(server, "/");
	addRoute(server, "/api");
	config.addServer(server);

	HttpRequestResolver			resolver;
	HttpRequestResolver::Result	result;

	result = resolver.resolve(config, listen,
		makeRequest("/other/path", "localhost"));
	assert_true(result.valid, "unmatched path should still be valid");
	assert_true(result.route != NULL,
		"root route should catch unmatched paths");
	assert_true(result.route->getPath() == "/",
		"root route should be selected when nothing else matches");
}

static void	test_safe_normal_path_is_accepted()
{
	Config			config;
	ListenConfig	listen("0.0.0.0", 8080);
	ServerConfig	server;

	server = makeServer("0.0.0.0", 8080, "");
	addRoute(server, "/");
	config.addServer(server);

	HttpRequestResolver			resolver;
	HttpRequestResolver::Result	result;

	result = resolver.resolve(config, listen,
		makeRequest("/static/style.css", "localhost"));
	assert_true(result.valid, "safe path should be accepted");
	assert_true(result.errorStatus == 0, "safe path should have no error status");
	assert_true(result.requestPath == "/static/style.css",
		"safe path should be stored as requestPath");
}

static void	test_traversal_path_is_rejected()
{
	Config			config;
	ListenConfig	listen("0.0.0.0", 8080);
	ServerConfig	server;

	server = makeServer("0.0.0.0", 8080, "");
	addRoute(server, "/");
	config.addServer(server);

	HttpRequestResolver	resolver;
	HttpRequestResolver::Result	result;

	result = resolver.resolve(config, listen,
		makeRequest("/../etc/passwd", "localhost"));
	assert_true(!result.valid, "traversal path should be rejected");
	assert_true(result.errorStatus == 400,
		"traversal path should produce 400");
}

static void	test_mid_path_traversal_is_rejected()
{
	Config			config;
	ListenConfig	listen("0.0.0.0", 8080);
	ServerConfig	server;

	server = makeServer("0.0.0.0", 8080, "");
	addRoute(server, "/");
	config.addServer(server);

	HttpRequestResolver	resolver;
	HttpRequestResolver::Result	result;

	result = resolver.resolve(config, listen,
		makeRequest("/foo/../bar", "localhost"));
	assert_true(!result.valid, "mid-path traversal should be rejected");
	assert_true(result.errorStatus == 400,
		"mid-path traversal should produce 400");
}

static void	test_triple_dot_segment_is_not_traversal()
{
	Config			config;
	ListenConfig	listen("0.0.0.0", 8080);
	ServerConfig	server;

	server = makeServer("0.0.0.0", 8080, "");
	addRoute(server, "/");
	config.addServer(server);

	HttpRequestResolver			resolver;
	HttpRequestResolver::Result	result;

	result = resolver.resolve(config, listen,
		makeRequest("/...", "localhost"));
	assert_true(result.valid, "triple dot segment should not be treated as traversal");
}

static void	test_query_string_is_stripped_for_route_matching()
{
	Config			config;
	ListenConfig	listen("0.0.0.0", 8080);
	ServerConfig	server;

	server = makeServer("0.0.0.0", 8080, "");
	addRoute(server, "/search");
	config.addServer(server);

	HttpRequestResolver			resolver;
	HttpRequestResolver::Result	result;

	result = resolver.resolve(config, listen,
		makeRequest("/search?q=hello", "localhost"));
	assert_true(result.valid, "target with query string should be valid");
	assert_true(result.route != NULL,
		"query string should not prevent route match");
	assert_true(result.requestPath == "/search",
		"query string should be stripped from requestPath");
}

static void	test_no_matching_server_returns_null_server()
{
	Config			config;
	ListenConfig	listen("0.0.0.0", 8080);
	ServerConfig	server;

	/* Server listens on a different port. */
	server = makeServer("0.0.0.0", 9090, "example.com");
	addRoute(server, "/");
	config.addServer(server);

	HttpRequestResolver			resolver;
	HttpRequestResolver::Result	result;

	result = resolver.resolve(config, listen,
		makeRequest("/", "example.com"));
	assert_true(result.valid,
		"no matching listener should still produce valid path result");
	assert_true(result.server == NULL,
		"no matching listener should leave server as null");
	assert_true(result.route == NULL,
		"no matching listener should leave route as null");
}

static void	test_filesystem_path_joins_root_and_relative_path()
{
	Config			config;
	ListenConfig	listen("0.0.0.0", 8080);
	ServerConfig	server;
	RouteConfig		route;

	route.setPath("/");
	route.setRoot("www");
	server = makeServer("0.0.0.0", 8080, "");
	server.addRoute(route);
	config.addServer(server);

	HttpRequestResolver			resolver;
	HttpRequestResolver::Result	result;

	result = resolver.resolve(config, listen,
		makeRequest("/index.html", "localhost"));
	assert_true(result.valid, "request should be valid");
	assert_true(result.filesystemPath == "www/index.html",
		"root route should prefix requestPath onto root");
}

static void	test_filesystem_path_strips_route_prefix()
{
	Config			config;
	ListenConfig	listen("0.0.0.0", 8080);
	ServerConfig	server;
	RouteConfig		route;

	route.setPath("/images");
	route.setRoot("www/static");
	server = makeServer("0.0.0.0", 8080, "");
	server.addRoute(route);
	config.addServer(server);

	HttpRequestResolver			resolver;
	HttpRequestResolver::Result	result;

	result = resolver.resolve(config, listen,
		makeRequest("/images/photo.jpg", "localhost"));
	assert_true(result.valid, "request should be valid");
	assert_true(result.filesystemPath == "www/static/photo.jpg",
		"route prefix should be stripped from filesystem path");
}

static void	test_filesystem_path_root_request_matches_root_exactly()
{
	Config			config;
	ListenConfig	listen("0.0.0.0", 8080);
	ServerConfig	server;
	RouteConfig		route;

	route.setPath("/");
	route.setRoot("www");
	server = makeServer("0.0.0.0", 8080, "");
	server.addRoute(route);
	config.addServer(server);

	HttpRequestResolver			resolver;
	HttpRequestResolver::Result	result;

	result = resolver.resolve(config, listen,
		makeRequest("/", "localhost"));
	assert_true(result.valid, "root request should be valid");
	assert_true(result.filesystemPath == "www/",
		"root request should map to root directory");
}

static void	test_filesystem_path_empty_when_no_root_configured()
{
	Config			config;
	ListenConfig	listen("0.0.0.0", 8080);
	ServerConfig	server;

	server = makeServer("0.0.0.0", 8080, "");
	addRoute(server, "/");
	config.addServer(server);

	HttpRequestResolver			resolver;
	HttpRequestResolver::Result	result;

	result = resolver.resolve(config, listen,
		makeRequest("/index.html", "localhost"));
	assert_true(result.valid, "request should be valid");
	assert_true(result.filesystemPath.empty(),
		"filesystem path should be empty when route has no root");
}

static void	test_filesystem_path_empty_when_no_route_matched()
{
	Config			config;
	ListenConfig	listen("0.0.0.0", 8080);
	ServerConfig	server;

	server = makeServer("0.0.0.0", 9090, "example.com");
	addRoute(server, "/");
	config.addServer(server);

	HttpRequestResolver			resolver;
	HttpRequestResolver::Result	result;

	result = resolver.resolve(config, listen,
		makeRequest("/index.html", "example.com"));
	assert_true(result.route == NULL, "route should be null");
	assert_true(result.filesystemPath.empty(),
		"filesystem path should be empty when no route matched");
}

static void	test_filesystem_path_deepest_prefix_wins()
{
	Config			config;
	ListenConfig	listen("0.0.0.0", 8080);
	ServerConfig	server;
	RouteConfig		root;
	RouteConfig		images;

	root.setPath("/");
	root.setRoot("www");
	images.setPath("/images");
	images.setRoot("www/img");
	server = makeServer("0.0.0.0", 8080, "");
	server.addRoute(root);
	server.addRoute(images);
	config.addServer(server);

	HttpRequestResolver			resolver;
	HttpRequestResolver::Result	result;

	result = resolver.resolve(config, listen,
		makeRequest("/images/photo.jpg", "localhost"));
	assert_true(result.valid, "request should be valid");
	assert_true(result.route != NULL && result.route->getPath() == "/images",
		"deepest matching route should be selected");
	assert_true(result.filesystemPath == "www/img/photo.jpg",
		"filesystem path should use the deepest matched route root");
}

int	main()
{
	try
	{
		test_host_header_selects_matching_server_name();
		test_no_host_falls_back_to_default_server();
		test_unmatched_host_falls_back_to_default_server();
		test_host_with_port_matches_server_name_without_port();
		test_longest_route_prefix_is_selected();
		test_route_boundary_is_respected();
		test_root_route_matches_when_nothing_else_does();
		test_safe_normal_path_is_accepted();
		test_traversal_path_is_rejected();
		test_mid_path_traversal_is_rejected();
		test_triple_dot_segment_is_not_traversal();
		test_query_string_is_stripped_for_route_matching();
		test_no_matching_server_returns_null_server();
		test_filesystem_path_joins_root_and_relative_path();
		test_filesystem_path_strips_route_prefix();
		test_filesystem_path_root_request_matches_root_exactly();
		test_filesystem_path_empty_when_no_root_configured();
		test_filesystem_path_empty_when_no_route_matched();
		test_filesystem_path_deepest_prefix_wins();
	}
	catch (const std::exception &error)
	{
		std::cerr << "test_http_request_resolver: "
			<< error.what() << std::endl;
		return (1);
	}
	std::cout << "test_http_request_resolver: OK" << std::endl;
	return (0);
}
