#include "http/HttpMethod.hpp"

HttpMethod	parseHttpMethod(const std::string &value)
{
	if (value == "GET")
		return (HTTP_METHOD_GET);
	if (value == "POST")
		return (HTTP_METHOD_POST);
	if (value == "DELETE")
		return (HTTP_METHOD_DELETE);
	return (HTTP_METHOD_UNKNOWN);
}

std::string	httpMethodToString(HttpMethod method)
{
	if (method == HTTP_METHOD_GET)
		return ("GET");
	if (method == HTTP_METHOD_POST)
		return ("POST");
	if (method == HTTP_METHOD_DELETE)
		return ("DELETE");
	return ("UNKNOWN");
}

bool	isSupportedHttpMethod(HttpMethod method)
{
	if (method == HTTP_METHOD_GET)
		return (true);
	if (method == HTTP_METHOD_POST)
		return (true);
	if (method == HTTP_METHOD_DELETE)
		return (true);
	return (false);
}

bool	containsHttpMethod(const std::vector<HttpMethod> &methods, HttpMethod method)
{
	std::vector<HttpMethod>::const_iterator	it;

	it = methods.begin();
	while (it != methods.end())
	{
		if (*it == method)
			return (true);
		++it;
	}
	return (false);
}