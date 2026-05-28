#include "config/ConfigParser.hpp"
#include "config/ConfigException.hpp"
#include "config/ConfigParserErrors.hpp"

/*
** Route grammar layer.
**
** This file turns location blocks into RouteConfig objects.
** It does not resolve routes or combine them with server defaults.
*/

void	ConfigParser::parseLocation(ServerConfig &server)
{
	server.addRoute(parseRoute());
}

RouteConfig	ConfigParser::parseRoute()
{
	RouteConfig			route;
	const ConfigToken&	path = expectWord(
			ConfigParserErrors::EXPECTED_LOCATION_PATH);

	route.setPath(path.getValue());
	expect(CONFIG_TOKEN_OPEN_BRACE,
		ConfigParserErrors::EXPECTED_LOCATION_OPEN_BRACE);
	expect(CONFIG_TOKEN_CLOSE_BRACE,
		ConfigParserErrors::EXPECTED_LOCATION_CLOSE_BRACE);

	return (route);
}
