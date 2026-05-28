#include "config/ConfigParser.hpp"
#include "config/ConfigParserErrors.hpp"
#include "config/ConfigException.hpp"

/*
** Server grammar layer.
**
** This file turns server-level tokens into ServerConfig fields.
** It does not validate filesystem paths, bind sockets, or resolve routes.
**
** Supported server directives:
** - listen
** - server_name
** - root
** - index
** - client_max_body_size
** - error_page
*/
Config	ConfigParser::parseConfig()
{
	Config	config;

	while (!isAtEnd())
		config.addServer(parseServer());

	return (config);
}

ServerConfig	ConfigParser::parseServer()
{
	ServerConfig			server;
	const ConfigToken&	token = expectWord(
			ConfigParserErrors::EXPECTED_SERVER_BLOCK);

	if (token.getValue() != "server")
		throw (ConfigException(ConfigParserErrors::EXPECTED_SERVER_BLOCK,
				token.getLine(), token.getColumn()));

	expect(CONFIG_TOKEN_OPEN_BRACE,
		ConfigParserErrors::EXPECTED_SERVER_OPEN_BRACE);

	while (!check(CONFIG_TOKEN_CLOSE_BRACE))
		parseServerDirective(server);
	expect(CONFIG_TOKEN_CLOSE_BRACE,
		ConfigParserErrors::EXPECTED_SERVER_CLOSE_BRACE);

	return (server);
}

void	ConfigParser::parseServerDirective(ServerConfig &server)
{
	const ConfigToken&	token = expectWord(
			ConfigParserErrors::UNKNOWN_SERVER_DIRECTIVE);

	if (token.getValue() == "listen")
		parseListen(server);
	else if (token.getValue() == "server_name")
		parseServerName(server);
	else if (token.getValue() == "root")
		parseRoot(server);
	else if (token.getValue() == "index")
		parseIndex(server);
	else if (token.getValue() == "client_max_body_size")
		parseClientMaxBodySize(server);
	else if (token.getValue() == "error_page")
		parseErrorPage(server);
	else
		throw (ConfigException(ConfigParserErrors::UNKNOWN_SERVER_DIRECTIVE,
			token.getLine(), token.getColumn()));
}

void	ConfigParser::parseListen(ServerConfig &server)
{
	const ConfigToken&	token = expectWord(ConfigParserErrors::EXPECTED_LISTEN_VALUE);

	server.addListen(parseListenValue(token));
	expect(CONFIG_TOKEN_SEMICOLON, ConfigParserErrors::EXPECTED_DIRECTIVE_SEMICOLON);
}

void	ConfigParser::parseServerName(ServerConfig &server)
{
	const ConfigToken*	token;

	token = &expectWord(ConfigParserErrors::EXPECTED_SERVER_NAME);
	while (true)
	{
		server.addServerName(token->getValue());
		if (match(CONFIG_TOKEN_SEMICOLON))
			break ;
		token = &expectWord(ConfigParserErrors::EXPECTED_SERVER_NAME);
	}
}

void	ConfigParser::parseRoot(ServerConfig &server)
{
	const ConfigToken&	token = expectWord(
			ConfigParserErrors::EXPECTED_ROOT_VALUE);

	server.setRoot(token.getValue());
	expect(CONFIG_TOKEN_SEMICOLON,
		ConfigParserErrors::EXPECTED_DIRECTIVE_SEMICOLON);
}

void	ConfigParser::parseIndex(ServerConfig &server)
{
	const ConfigToken&	token = expectWord(
		ConfigParserErrors::EXPECTED_INDEX_VALUE);

	server.setIndex(token.getValue());
	expect(CONFIG_TOKEN_SEMICOLON,
		ConfigParserErrors::EXPECTED_DIRECTIVE_SEMICOLON);
}

void	ConfigParser::parseClientMaxBodySize(ServerConfig &server)
{
	const ConfigToken&	token = expectWord(
			ConfigParserErrors::EXPECTED_BODY_SIZE_VALUE);

	server.setClientMaxBodySize(parseSize(token.getValue(), token));
	expect(CONFIG_TOKEN_SEMICOLON,
		ConfigParserErrors::EXPECTED_DIRECTIVE_SEMICOLON);
}

/*
** error_page accepts one or more status codes followed by one path:
**
**     error_page 404 /errors/404.html;
**     error_page 403 404 500 /errors/default.html;
**
** The last word before ';' is treated as the path. Every previous word
** must be a valid status code and is mapped to that same path.
*/
void	ConfigParser::parseErrorPage(ServerConfig &server)
{
	std::vector<unsigned int>	statuses;
	ConfigToken					token;
	std::string					path;

	token = expectWord(ConfigParserErrors::EXPECTED_ERROR_STATUS);
	while (!check(CONFIG_TOKEN_SEMICOLON))
	{
		statuses.push_back(parseStatusCode(token.getValue(), token));
		token = expectWord(ConfigParserErrors::EXPECTED_ERROR_PAGE_PATH);
	}
	if (statuses.empty())
		throw (ConfigException(ConfigParserErrors::EXPECTED_ERROR_STATUS,
				token.getLine(), token.getColumn()));
	path = token.getValue();
	addErrorPages(server, statuses, path);
	expect(CONFIG_TOKEN_SEMICOLON,
		ConfigParserErrors::EXPECTED_DIRECTIVE_SEMICOLON);
}

void	ConfigParser::addErrorPages(ServerConfig &server,
	const std::vector<unsigned int> &statuses,
	const std::string &path)
{
	std::vector<unsigned int>::const_iterator	it;

	it = statuses.begin();
	while (it != statuses.end())
	{
		server.addErrorPage(*it, path);
		++it;
	}
}
