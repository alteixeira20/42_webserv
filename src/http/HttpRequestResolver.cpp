#include "http/HttpRequestResolver.hpp"

HttpRequestResolver::HttpRequestResolver()
	: _resolver()
{
}

HttpRequestResolver::HttpRequestResolver(const HttpRequestResolver &other)
	: _resolver(other._resolver)
{
}

HttpRequestResolver	&HttpRequestResolver::operator=(
	const HttpRequestResolver &other)
{
	if (this != &other)
		_resolver = other._resolver;

	return (*this);
}

HttpRequestResolver::~HttpRequestResolver()
{
}

HttpRequestResolver::Result	HttpRequestResolver::resolve(
	const Config &config,
	const ListenConfig &listen,
	const HttpRequest &request) const
{
	Result		result;
	std::string	host;
	std::string	path;

	result.server = NULL;
	result.route = NULL;
	result.filesystemPath = "";
	result.valid = false;
	result.errorStatus = 0;

	path = extractPath(request.getTarget());
	if (path.empty() || path[0] != '/')
	{
		result.errorStatus = 400;
		return (result);
	}
	if (hasTraversalSegment(path))
	{
		result.errorStatus = 400;
		return (result);
	}

	host = extractHost(request.getHeader("host"));
	result.server = _resolver.findServer(config, listen, host);
	if (result.server != NULL)
		result.route = _resolver.findRoute(*result.server, path);

	result.requestPath = path;
	if (result.route != NULL)
		result.filesystemPath = buildFilesystemPath(*result.route, path);
	result.valid = true;

	return (result);
}

/*
** Strips the port suffix from a Host header value (e.g. "example.com:8080"
** becomes "example.com"). Uses rfind so plain hostnames pass through unchanged.
** IPv6 bracket notation is not supported.
*/
std::string	HttpRequestResolver::extractHost(
	const std::string &hostHeader) const
{
	std::string::size_type	colon;

	colon = hostHeader.rfind(':');
	if (colon == std::string::npos)
		return (hostHeader);

	return (hostHeader.substr(0, colon));
}

std::string	HttpRequestResolver::extractPath(
	const std::string &target) const
{
	std::string::size_type	queryPos;
	std::string::size_type	fragmentPos;
	std::string::size_type	endPos;

	queryPos = target.find('?');
	fragmentPos = target.find('#');
	endPos = target.length();

	if (queryPos != std::string::npos)
		endPos = queryPos;
	if (fragmentPos != std::string::npos && fragmentPos < endPos)
		endPos = fragmentPos;

	return (target.substr(0, endPos));
}

bool	HttpRequestResolver::hasTraversalSegment(
	const std::string &path) const
{
	std::string::size_type	start;
	std::string::size_type	end;
	std::string				segment;

	start = 0;
	while (start < path.length())
	{
		if (path[start] == '/')
			++start;
		end = path.find('/', start);
		if (end == std::string::npos)
			end = path.length();
		segment = path.substr(start, end - start);
		if (segment == "..")
			return (true);
		start = end;
	}

	return (false);
}

/*
** Builds a filesystem candidate path from the route root and the request path.
** The route prefix is stripped from requestPath to produce the relative part,
** which is then joined onto the root. No filesystem access is performed.
**
** Returns an empty string when the route has no root configured.
*/
std::string	HttpRequestResolver::buildFilesystemPath(
	const RouteConfig &route,
	const std::string &requestPath) const
{
	std::string	root;
	std::string	prefix;
	std::string	relative;

	root = route.getRoot();
	if (root.empty())
		return ("");

	while (root.length() > 1 && root[root.length() - 1] == '/')
		root.erase(root.length() - 1, 1);

	prefix = route.getPath();
	while (prefix.length() > 1 && prefix[prefix.length() - 1] == '/')
		prefix.erase(prefix.length() - 1, 1);

	/*
	** When the route prefix is "/", strip nothing: the relative part is the
	** full requestPath (e.g. "/index.html"), so root + relative concatenates
	** cleanly. For all other prefixes the relative part begins with "/" already.
	*/
	if (prefix == "/")
		relative = requestPath;
	else if (requestPath.length() >= prefix.length())
		relative = requestPath.substr(prefix.length());

	return (root + relative);
}
