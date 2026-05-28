#include <sstream>
#include "config/ConfigValidator.hpp"
#include "config/ConfigException.hpp"
#include "config/ConfigValidationErrors.hpp"

/*
** Semantic validation layer.
**
** The parser checks syntax and builds typed objects. The validator checks
** whether the parsed configuration is unambiguous enough for runtime use.
**
** Multiple servers may share a listen endpoint for virtual hosting, but each
** endpoint may have only one unnamed default server and unique server names.
*/

ConfigValidator::ConfigValidator()
{
}

ConfigValidator::ConfigValidator(const ConfigValidator &other)
{
	(void)other;
}

ConfigValidator&	ConfigValidator::operator=(const ConfigValidator &other)
{
	(void)other;
	return (*this);
}

ConfigValidator::~ConfigValidator()
{
}

void	ConfigValidator::validate(const Config &config) const
{
	std::vector<ServerConfig>::const_iterator	it;
	VirtualHostMap								virtualHosts;

	if (config.getServers().empty())
		throw (ConfigException(ConfigValidationErrors::EMPTY_CONFIG, 0, 0));

	it = config.getServers().begin();
	while (it != config.getServers().end())
	{
		validateServer(*it, virtualHosts);
		++it;
	}
}

void	ConfigValidator::validateServer(const ServerConfig &server,
	VirtualHostMap &virtualHosts) const
{
	if (server.getListens().empty())
		throw (ConfigException(ConfigValidationErrors::SERVER_WITHOUT_LISTEN,
				0, 0));

	validateServerListens(server);
	validateVirtualHosts(server, virtualHosts);
	validateRoutes(server);
}

void	ConfigValidator::validateServerListens(
	const ServerConfig &server) const
{
	std::vector<ListenConfig>::const_iterator	it;
	StringSet									listenKeys;
	std::string									key;

	it = server.getListens().begin();
	while (it != server.getListens().end())
	{
		key = listenKey(*it);
		if (listenKeys.find(key) != listenKeys.end())
			throw (ConfigException(
					ConfigValidationErrors::DUPLICATE_LISTEN_IN_SERVER,
					0, 0));
		listenKeys.insert(key);
		++it;
	}
}

void	ConfigValidator::validateVirtualHosts(const ServerConfig &server,
	VirtualHostMap &virtualHosts) const
{
	std::vector<ListenConfig>::const_iterator	listenIt;
	std::vector<std::string>::const_iterator	nameIt;
	std::string									key;

	listenIt = server.getListens().begin();
	while (listenIt != server.getListens().end())
	{
		key = listenKey(*listenIt);
		if (server.getServerNames().empty())
			validateVirtualHostName(key, "", virtualHosts);
		else
		{
			nameIt = server.getServerNames().begin();
			while (nameIt != server.getServerNames().end())
			{
				validateVirtualHostName(key, *nameIt, virtualHosts);
				++nameIt;
			}
		}
		++listenIt;
	}
}

void	ConfigValidator::validateVirtualHostName(
	const std::string &listenKey, const std::string &serverName,
	VirtualHostMap &virtualHosts) const
{
	StringSet	&names = virtualHosts[listenKey];

	if (names.find(serverName) != names.end())
	{
		if (serverName.empty())
			throw (ConfigException(
					ConfigValidationErrors::DUPLICATE_DEFAULT_SERVER,
					0, 0));
		throw (ConfigException(
				ConfigValidationErrors::DUPLICATE_SERVER_NAME, 0, 0));
	}

	names.insert(serverName);
}

void	ConfigValidator::validateRoutes(const ServerConfig &server) const
{
	std::vector<RouteConfig>::const_iterator	it;
	StringSet									routePaths;

	it = server.getRoutes().begin();
	while (it != server.getRoutes().end())
	{
		validateRoute(*it, routePaths);
		++it;
	}
}

void	ConfigValidator::validateRoute(const RouteConfig &route,
	StringSet &routePaths) const
{
	if (route.getPath().empty() || route.getPath()[0] != '/')
		throw (ConfigException(ConfigValidationErrors::INVALID_ROUTE_PATH,
				0, 0));

	if (routePaths.find(route.getPath()) != routePaths.end())
		throw (ConfigException(ConfigValidationErrors::DUPLICATE_ROUTE_PATH,
				0, 0));

	routePaths.insert(route.getPath());
}

std::string	ConfigValidator::listenKey(const ListenConfig &listen) const
{
	std::stringstream	stream;

	stream << listen.getHost() << ":" << listen.getPort();

	return (stream.str());
}
