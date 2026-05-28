#include <cstdlib>
#include "config/ConfigParser.hpp"
#include "config/ConfigParserErrors.hpp"
#include "config/ConfigException.hpp"

/*
** Value conversion layer.
**
** This file converts directive words into typed config values.
** It is shared by server parsing now and route parsing later.
**
** It deliberately does not check filesystem existence or runtime behavior.
*/

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

std::size_t	ConfigParser::parseSize(const std::string &value,
	const ConfigToken &token) const
{
	unsigned long	size;

	if (!isOnlyDigits(value))
		throw (ConfigException(ConfigParserErrors::INVALID_BODY_SIZE,
				token.getLine(), token.getColumn()));

	size = std::strtoul(value.c_str(), NULL, 10);

	if (size == 0)
		throw (ConfigException(ConfigParserErrors::INVALID_BODY_SIZE,
				token.getLine(), token.getColumn()));

	return (static_cast<std::size_t>(size));
}

unsigned int	ConfigParser::parseStatusCode(const std::string &value,
	const ConfigToken &token) const
{
	long	status;

	if (!isOnlyDigits(value))
		throw (ConfigException(ConfigParserErrors::INVALID_ERROR_STATUS,
				token.getLine(), token.getColumn()));

	status = std::strtol(value.c_str(), NULL, 10);

	if (status < 300 || status > 599)
		throw (ConfigException(ConfigParserErrors::INVALID_ERROR_STATUS,
				token.getLine(), token.getColumn()));

	return (static_cast<unsigned int>(status));
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
