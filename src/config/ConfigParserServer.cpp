#include <cstdlib>
#include "config/ConfigParser.hpp"
#include "config/ConfigParserErrors.hpp"
#include "config/ConfigException.hpp"

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

ListenConfig	ConfigParser::parseListenValue(const ConfigToken &token) const
{
	std::string				value;
	std::string				host;
	std::string				portText;
	std::string::size_type	separator;

	value = token.getValue();
	separator = value.find(':');

	if (separator == std::string::npos)
		return (ListenConfig("0.0.0.0", parsePort(value, token)));
	
	if (separator == 0 || separator == value.length() - 1)
		throw (ConfigException(ConfigParserErrors::INVALID_LISTEN_VALUE,
			token.getLine(), token.getColumn()));
	host = value.substr(0, separator);
	portText = value.substr(separator + 1);

	return (ListenConfig(host, parsePort(portText, token)));
}

unsigned int	ConfigParser::parsePort(const std::string &value,
	const ConfigToken &token) const
{
	long	port;

	if (!isOnlyDigits(value))
		throw (ConfigException(ConfigParserErrors::INVALID_LISTEN_PORT,
			token.getLine(), token.getColumn()));

	port = std::strtol(value.c_str(), NULL, 10);

	if (port <= 0 || port > 65535)
		throw (ConfigException(ConfigParserErrors::INVALID_LISTEN_PORT,
			token.getLine(), token.getColumn()));
	
	return (static_cast<unsigned int>(port));
}

bool	ConfigParser::isOnlyDigits(const std::string &value) const
{
	std::string::size_type	index;

	if (value.empty())
		return (false);
	index = 0;
	while (index < value.length())
	{
		if (value[index] < '0' || value[index] > '9')
			return (false);
		++index;
	}
	return (true);
}
