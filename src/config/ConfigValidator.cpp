#include <sstream>
#include "config/ConfigValidator.hpp"
#include "config/ConfigException.hpp"
#include "config/ConfigValidationErrors.hpp"

/*
** Semantic validation layer.
**
** The parser checks syntax and builds typed objects.
** The validator checks whether the parsed configuration is usable by the
** runtime without inventing missing critical data.
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
	StringSet									listenKeys;

	if (config.getServers().empty())
		throw (ConfigException(ConfigValidationErrors::EMPTY_CONFIG, 0, 0));

	it = config.getServers().begin();
	while (it != config.getServers().end())
	{
		validateServer(*it, listenKeys);
		++it;
	}
}

void	ConfigValidator::validateServer(const ServerConfig &server,
	StringSet &listenKeys) const
{
	std::vector<ListenConfig>::const_iterator	it;

	if (server.getListens().empty())
		throw (ConfigException(ConfigValidationErrors::SERVER_WITHOUT_LISTEN,
				0, 0));
	it = server.getListens().begin();
	while (it != server.getListens().end())
	{
		validateListen(*it, listenKeys);
		++it;
	}
	validateRoutes(server);
}

void	ConfigValidator::validateListen(const ListenConfig &listen,
	StringSet &listenKeys) const
{
	std::string	key;

	key = listenKey(listen);
	if (listenKeys.find(key) != listenKeys.end())
		throw (ConfigException(ConfigValidationErrors::DUPLICATE_LISTEN,
				0, 0));
	listenKeys.insert(key);
}

void	ConfigValidator::validateRoutes(const ServerConfig &server) const
{
	std::vector<RouteConfig>::const_iterator	it;
	StringSet								routePaths;

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
