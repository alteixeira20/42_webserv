#include "config/RouteConfig.hpp"
#include "config/ServerConfig.hpp"
#include "http/HttpMethod.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpRequestEvaluator.hpp"
#include "http/HttpRequestResolver.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

static void	assert_true(bool condition, const std::string &message)
{
	if (!condition)
		throw std::runtime_error(message);
}

static HttpRequest	makeRequest(HttpMethod method, const std::string &body)
{
	HttpRequest	request;

	request.setMethod(method);
	request.setTarget("/path");
	request.setVersion("HTTP/1.1");
	request.setHeader("host", "localhost");
	if (!body.empty())
		request.setBody(body);

	return (request);
}

static HttpRequestResolver::Result	makeInvalidResult(unsigned int errorStatus)
{
	HttpRequestResolver::Result	result;

	result.server = NULL;
	result.route = NULL;
	result.requestPath = "";
	result.valid = false;
	result.errorStatus = errorStatus;

	return (result);
}

static HttpRequestResolver::Result	makeValidResult(
	const ServerConfig *server,
	const RouteConfig *route,
	const std::string &requestPath)
{
	HttpRequestResolver::Result	result;

	result.server = server;
	result.route = route;
	result.requestPath = requestPath;
	result.valid = true;
	result.errorStatus = 0;

	return (result);
}

static void	test_invalid_resolver_result_produces_400()
{
	HttpRequestEvaluator			evaluator;
	HttpRequestEvaluator::Decision	decision;

	decision = evaluator.evaluate(makeRequest(HTTP_METHOD_GET, ""),
		makeInvalidResult(400));
	assert_true(decision.action == HttpRequestEvaluator::ERROR,
		"invalid result should produce ERROR action");
	assert_true(decision.statusCode == 400,
		"invalid result with errorStatus 400 should produce 400");
}

static void	test_invalid_resolver_result_uses_error_status_from_resolver()
{
	HttpRequestEvaluator			evaluator;
	HttpRequestEvaluator::Decision	decision;

	decision = evaluator.evaluate(makeRequest(HTTP_METHOD_GET, ""),
		makeInvalidResult(414));
	assert_true(decision.statusCode == 414,
		"invalid result should forward errorStatus from resolver");
}

static void	test_invalid_result_with_zero_error_status_defaults_to_400()
{
	HttpRequestEvaluator			evaluator;
	HttpRequestEvaluator::Decision	decision;

	decision = evaluator.evaluate(makeRequest(HTTP_METHOD_GET, ""),
		makeInvalidResult(0));
	assert_true(decision.statusCode == 400,
		"zero errorStatus in invalid result should default to 400");
}

static void	test_null_server_produces_404()
{
	HttpRequestEvaluator			evaluator;
	HttpRequestEvaluator::Decision	decision;

	decision = evaluator.evaluate(makeRequest(HTTP_METHOD_GET, ""),
		makeValidResult(NULL, NULL, "/path"));
	assert_true(decision.action == HttpRequestEvaluator::ERROR,
		"null server should produce ERROR action");
	assert_true(decision.statusCode == 404,
		"null server should produce 404");
}

static void	test_null_route_with_server_produces_404()
{
	ServerConfig					server;
	HttpRequestEvaluator			evaluator;
	HttpRequestEvaluator::Decision	decision;

	decision = evaluator.evaluate(makeRequest(HTTP_METHOD_GET, ""),
		makeValidResult(&server, NULL, "/path"));
	assert_true(decision.action == HttpRequestEvaluator::ERROR,
		"null route with server should produce ERROR action");
	assert_true(decision.statusCode == 404,
		"null route with server should produce 404");
}

static void	test_redirect_route_produces_redirect_decision()
{
	ServerConfig					server;
	RouteConfig						route;
	HttpRequestEvaluator			evaluator;
	HttpRequestEvaluator::Decision	decision;

	route.setPath("/old");
	route.setRedirect(301, "/new");
	decision = evaluator.evaluate(makeRequest(HTTP_METHOD_GET, ""),
		makeValidResult(&server, &route, "/old"));
	assert_true(decision.action == HttpRequestEvaluator::REDIRECT,
		"redirect route should produce REDIRECT action");
	assert_true(decision.statusCode == 301,
		"redirect route should forward configured redirect status");
	assert_true(decision.redirectLocation == "/new",
		"redirect route should forward configured redirect target");
}

static void	test_redirect_route_uses_302_when_configured()
{
	ServerConfig					server;
	RouteConfig						route;
	HttpRequestEvaluator			evaluator;
	HttpRequestEvaluator::Decision	decision;

	route.setPath("/temp");
	route.setRedirect(302, "/elsewhere");
	decision = evaluator.evaluate(makeRequest(HTTP_METHOD_GET, ""),
		makeValidResult(&server, &route, "/temp"));
	assert_true(decision.action == HttpRequestEvaluator::REDIRECT,
		"redirect route should produce REDIRECT");
	assert_true(decision.statusCode == 302,
		"redirect route should use configured 302 status");
	assert_true(decision.redirectLocation == "/elsewhere",
		"redirect route should forward configured location");
}

static void	test_redirect_is_evaluated_before_method_gating()
{
	ServerConfig					server;
	RouteConfig						route;
	HttpRequestEvaluator			evaluator;
	HttpRequestEvaluator::Decision	decision;

	route.setPath("/redir");
	route.setRedirect(301, "/other");
	route.addAllowedMethod(HTTP_METHOD_GET);
	decision = evaluator.evaluate(makeRequest(HTTP_METHOD_POST, ""),
		makeValidResult(&server, &route, "/redir"));
	assert_true(decision.action == HttpRequestEvaluator::REDIRECT,
		"redirect should be returned before method check");
	assert_true(decision.statusCode == 301,
		"redirect should win over method not allowed");
}

static void	test_disallowed_method_produces_405()
{
	ServerConfig					server;
	RouteConfig						route;
	HttpRequestEvaluator			evaluator;
	HttpRequestEvaluator::Decision	decision;

	route.setPath("/get-only");
	route.addAllowedMethod(HTTP_METHOD_GET);
	decision = evaluator.evaluate(makeRequest(HTTP_METHOD_POST, ""),
		makeValidResult(&server, &route, "/get-only"));
	assert_true(decision.action == HttpRequestEvaluator::ERROR,
		"disallowed method should produce ERROR action");
	assert_true(decision.statusCode == 405,
		"disallowed method should produce 405");
}

static void	test_allowed_method_continues()
{
	ServerConfig					server;
	RouteConfig						route;
	HttpRequestEvaluator			evaluator;
	HttpRequestEvaluator::Decision	decision;

	route.setPath("/api");
	route.addAllowedMethod(HTTP_METHOD_POST);
	decision = evaluator.evaluate(makeRequest(HTTP_METHOD_POST, ""),
		makeValidResult(&server, &route, "/api"));
	assert_true(decision.action == HttpRequestEvaluator::CONTINUE,
		"allowed method should produce CONTINUE action");
}

static void	test_empty_allowed_methods_permits_any_method()
{
	ServerConfig					server;
	RouteConfig						route;
	HttpRequestEvaluator			evaluator;
	HttpRequestEvaluator::Decision	decision;

	route.setPath("/open");
	decision = evaluator.evaluate(makeRequest(HTTP_METHOD_DELETE, ""),
		makeValidResult(&server, &route, "/open"));
	assert_true(decision.action == HttpRequestEvaluator::CONTINUE,
		"empty allowed_methods should permit any method");
}

static void	test_body_exceeding_server_limit_produces_413()
{
	ServerConfig					server;
	RouteConfig						route;
	HttpRequestEvaluator			evaluator;
	HttpRequestEvaluator::Decision	decision;
	std::string						body;

	server.setClientMaxBodySize(10);
	route.setPath("/upload");
	body = "12345678901";
	decision = evaluator.evaluate(makeRequest(HTTP_METHOD_POST, body),
		makeValidResult(&server, &route, "/upload"));
	assert_true(decision.action == HttpRequestEvaluator::ERROR,
		"body over limit should produce ERROR action");
	assert_true(decision.statusCode == 413,
		"body over limit should produce 413");
}

static void	test_body_at_server_limit_continues()
{
	ServerConfig					server;
	RouteConfig						route;
	HttpRequestEvaluator			evaluator;
	HttpRequestEvaluator::Decision	decision;
	std::string						body;

	server.setClientMaxBodySize(10);
	route.setPath("/upload");
	body = "1234567890";
	decision = evaluator.evaluate(makeRequest(HTTP_METHOD_POST, body),
		makeValidResult(&server, &route, "/upload"));
	assert_true(decision.action == HttpRequestEvaluator::CONTINUE,
		"body at limit should produce CONTINUE action");
}

static void	test_empty_body_always_continues()
{
	ServerConfig					server;
	RouteConfig						route;
	HttpRequestEvaluator			evaluator;
	HttpRequestEvaluator::Decision	decision;

	server.setClientMaxBodySize(0);
	route.setPath("/");
	decision = evaluator.evaluate(makeRequest(HTTP_METHOD_GET, ""),
		makeValidResult(&server, &route, "/"));
	assert_true(decision.action == HttpRequestEvaluator::CONTINUE,
		"empty body should always continue regardless of limit");
}

static void	test_continue_decision_has_zero_status()
{
	ServerConfig					server;
	RouteConfig						route;
	HttpRequestEvaluator			evaluator;
	HttpRequestEvaluator::Decision	decision;

	route.setPath("/");
	decision = evaluator.evaluate(makeRequest(HTTP_METHOD_GET, ""),
		makeValidResult(&server, &route, "/"));
	assert_true(decision.statusCode == 0,
		"CONTINUE decision should have status 0");
}

int	main()
{
	try
	{
		test_invalid_resolver_result_produces_400();
		test_invalid_resolver_result_uses_error_status_from_resolver();
		test_invalid_result_with_zero_error_status_defaults_to_400();
		test_null_server_produces_404();
		test_null_route_with_server_produces_404();
		test_redirect_route_produces_redirect_decision();
		test_redirect_route_uses_302_when_configured();
		test_redirect_is_evaluated_before_method_gating();
		test_disallowed_method_produces_405();
		test_allowed_method_continues();
		test_empty_allowed_methods_permits_any_method();
		test_body_exceeding_server_limit_produces_413();
		test_body_at_server_limit_continues();
		test_empty_body_always_continues();
		test_continue_decision_has_zero_status();
	}
	catch (const std::exception &error)
	{
		std::cerr << "test_http_request_evaluator: "
			<< error.what() << std::endl;
		return (1);
	}
	std::cout << "test_http_request_evaluator: OK" << std::endl;
	return (0);
}
