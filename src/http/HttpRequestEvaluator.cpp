#include "http/HttpRequestEvaluator.hpp"
#include "http/HttpMethod.hpp"

HttpRequestEvaluator::HttpRequestEvaluator()
{
}

HttpRequestEvaluator::HttpRequestEvaluator(const HttpRequestEvaluator &)
{
}

HttpRequestEvaluator	&HttpRequestEvaluator::operator=(
	const HttpRequestEvaluator &)
{
	return (*this);
}

HttpRequestEvaluator::~HttpRequestEvaluator()
{
}

HttpRequestEvaluator::Decision	HttpRequestEvaluator::evaluate(
	const HttpRequest &request,
	const HttpRequestResolver::Result &resolved) const
{
	Decision						result;
	const std::vector<HttpMethod>	*methods;

	result.action = CONTINUE;
	result.statusCode = 0;

	if (!resolved.valid)
	{
		result.action = ERROR;
		result.statusCode = resolved.errorStatus ? resolved.errorStatus : 400;
		return (result);
	}

	if (resolved.server == NULL || resolved.route == NULL)
	{
		result.action = ERROR;
		result.statusCode = 404;
		return (result);
	}

	if (resolved.route->hasRedirect())
	{
		result.action = REDIRECT;
		result.statusCode = resolved.route->getRedirectStatus();
		result.redirectLocation = resolved.route->getRedirectTarget();
		return (result);
	}

	/*
	** Empty allowed_methods means all methods are permitted. Only gate the
	** request when the route explicitly restricts the allowed set.
	*/
	methods = &resolved.route->getAllowedMethods();
	if (!methods->empty() && !containsHttpMethod(*methods, request.getMethod()))
	{
		result.action = ERROR;
		result.statusCode = 405;
		return (result);
	}

	if (request.bodySize() > resolved.server->getClientMaxBodySize())
	{
		result.action = ERROR;
		result.statusCode = 413;
		return (result);
	}

	return (result);
}
