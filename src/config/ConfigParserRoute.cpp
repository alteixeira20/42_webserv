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

	while (!check(CONFIG_TOKEN_CLOSE_BRACE))
		parseRouteDirective(route);

	expect(CONFIG_TOKEN_CLOSE_BRACE,
		ConfigParserErrors::EXPECTED_LOCATION_CLOSE_BRACE);

	return (route);
}

void	ConfigParser::parseRouteDirective(RouteConfig &route)
{
	const ConfigToken&	token = expectWord(
			ConfigParserErrors::UNKNOWN_ROUTE_DIRECTIVE);

	if (token.getValue() == "root")
		parseRouteRoot(route);
	else if (token.getValue() == "index")
		parseRouteIndex(route);
	else if (token.getValue() == "autoindex")
		parseAutoIndex(route);
	else if (token.getValue() == "allowed_methods")
		parseAllowedMethods(route);
	else if (token.getValue() == "redirect")
		parseRedirect(route);
	else if (token.getValue() == "upload_dir")
		parseUploadDir(route);
	else if (token.getValue() == "cgi")
		parseCgi(route);
	else
		throw (ConfigException(ConfigParserErrors::UNKNOWN_ROUTE_DIRECTIVE,
				token.getLine(), token.getColumn()));
}

void	ConfigParser::parseRouteRoot(RouteConfig &route)
{
	const ConfigToken&	token = expectWord(
			ConfigParserErrors::EXPECTED_ROUTE_ROOT_VALUE);

	route.setRoot(token.getValue());

	expect(CONFIG_TOKEN_SEMICOLON,
		ConfigParserErrors::EXPECTED_DIRECTIVE_SEMICOLON);
}

void	ConfigParser::parseRouteIndex(RouteConfig &route)
{
	const ConfigToken&	token = expectWord(
			ConfigParserErrors::EXPECTED_ROUTE_INDEX_VALUE);

	route.setIndex(token.getValue());

	expect(CONFIG_TOKEN_SEMICOLON,
		ConfigParserErrors::EXPECTED_DIRECTIVE_SEMICOLON);
}

void	ConfigParser::parseAutoIndex(RouteConfig &route)
{
	const ConfigToken&	token = expectWord(
			ConfigParserErrors::EXPECTED_AUTOINDEX_VALUE);

	route.setAutoIndex(parseBoolean(token.getValue(), token));

	expect(CONFIG_TOKEN_SEMICOLON,
		ConfigParserErrors::EXPECTED_DIRECTIVE_SEMICOLON);
}

void	ConfigParser::parseAllowedMethods(RouteConfig &route)
{
	const ConfigToken*	token;

	token = &expectWord(ConfigParserErrors::EXPECTED_ALLOWED_METHOD);

	while (true)
	{
		route.addAllowedMethod(parseAllowedMethod(*token));
		if (match(CONFIG_TOKEN_SEMICOLON))
			break ;
		token = &expectWord(ConfigParserErrors::EXPECTED_ALLOWED_METHOD);
	}
}

void	ConfigParser::parseRedirect(RouteConfig &route)
{
	const ConfigToken&	status = expectWord(
			ConfigParserErrors::EXPECTED_REDIRECT_STATUS);
	const ConfigToken&	target = expectWord(
			ConfigParserErrors::EXPECTED_REDIRECT_TARGET);

	route.setRedirect(parseRedirectStatus(status.getValue(), status),
		target.getValue());

	expect(CONFIG_TOKEN_SEMICOLON,
		ConfigParserErrors::EXPECTED_DIRECTIVE_SEMICOLON);
}

void	ConfigParser::parseUploadDir(RouteConfig &route)
{
	const ConfigToken&	token = expectWord(
			ConfigParserErrors::EXPECTED_UPLOAD_DIR);

	route.setUploadDir(token.getValue());

	expect(CONFIG_TOKEN_SEMICOLON,
		ConfigParserErrors::EXPECTED_DIRECTIVE_SEMICOLON);
}

void	ConfigParser::parseCgi(RouteConfig &route)
{
	const ConfigToken&	extension = expectWord(
			ConfigParserErrors::EXPECTED_CGI_EXTENSION);
	const ConfigToken&	executable = expectWord(
			ConfigParserErrors::EXPECTED_CGI_EXECUTABLE);

	route.addCgiHandler(extension.getValue(), executable.getValue());

	expect(CONFIG_TOKEN_SEMICOLON,
		ConfigParserErrors::EXPECTED_DIRECTIVE_SEMICOLON);
}
