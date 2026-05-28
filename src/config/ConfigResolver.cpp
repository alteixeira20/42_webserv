#include "config/ConfigResolver.hpp"

/*
** Runtime config resolution layer.
**
** Server resolution first filters by listener endpoint, then prefers an
** exact Host/server_name match. If no name matches, the first server for
** that listener acts as the default server.
**
** Route resolution uses longest path-prefix matching with segment
** boundaries, so "/img" matches "/img/logo.png" but not "/img2".
*/

ConfigResolver::ConfigResolver()
{
}

ConfigResolver::ConfigResolver(const ConfigResolver &other)
{
	(void)other;
}

ConfigResolver&	ConfigResolver::operator=(const ConfigResolver &other)
{
	(void)other;
	return (*this);
}

ConfigResolver::~ConfigResolver()
{
}

const ServerConfig*	ConfigResolver::findServer(const Config &config,
	const ListenConfig &listen, const std::string &host) const
{
	std::vector<ServerConfig>::const_iterator	it;
	const ServerConfig*							defaultServer;

	defaultServer = NULL;

	it = config.getServers().begin();
	while (it != config.getServers().end())
	{
		if (serverListensOn(*it, listen))
		{
			if (defaultServer == NULL)
				defaultServer = &(*it);
			if (serverNameMatches(*it, host))
				return (&(*it));
		}
		++it;
	}

	return (defaultServer);
}

const RouteConfig*	ConfigResolver::findRoute(const ServerConfig &server,
	const std::string &path) const
{
	std::vector<RouteConfig>::const_iterator	it;
	const RouteConfig*						bestRoute;
	std::string::size_type					bestLength;

	bestRoute = NULL;
	bestLength = 0;

	it = server.getRoutes().begin();
	while (it != server.getRoutes().end())
	{
		if (routeMatchesPath(it->getPath(), path)
			&& it->getPath().length() >= bestLength)
		{
			bestRoute = &(*it);
			bestLength = it->getPath().length();
		}
		++it;
	}

	return (bestRoute);
}

bool	ConfigResolver::serverListensOn(const ServerConfig &server,
	const ListenConfig &listen) const
{
	std::vector<ListenConfig>::const_iterator	it;

	it = server.getListens().begin();
	while (it != server.getListens().end())
	{
		if (listenMatches(*it, listen))
			return (true);
		++it;
	}

	return (false);
}

bool	ConfigResolver::listenMatches(const ListenConfig &left,
	const ListenConfig &right) const
{
	return (left.getHost() == right.getHost()
		&& left.getPort() == right.getPort());
}

bool	ConfigResolver::serverNameMatches(const ServerConfig &server,
	const std::string &host) const
{
	std::vector<std::string>::const_iterator	it;

	it = server.getServerNames().begin();
	while (it != server.getServerNames().end())
	{
		if (*it == host)
			return (true);
		++it;
	}

	return (false);
}

bool	ConfigResolver::routeMatchesPath(const std::string &routePath,
	const std::string &requestPath) const
{
	if (routePath == "/")
		return (true);

	if (requestPath.compare(0, routePath.length(), routePath) != 0)
		return (false);

	if (requestPath.length() == routePath.length())
		return (true);

	return (requestPath[routePath.length()] == '/');
}
