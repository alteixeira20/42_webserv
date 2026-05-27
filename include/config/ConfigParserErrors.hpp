#ifndef CONFIG_PARSER_ERRORS_HPP
# define CONFIG_PARSER_ERRORS_HPP

/*
** Centralized messages for ConfigParser syntax errors.
** Keeping them here avoids duplicated strings across parser functions.
*/
namespace ConfigParserErrors
{
	static const char* const COULD_NOT_OPEN_FILE =
		"could not open config file";
	static const char* const COULD_NOT_READ_FILE =
		"could not read config file";
	static const char* const EXPECTED_SERVER_BLOCK =
		"expected 'server' block";
	static const char* const EXPECTED_SERVER_OPEN_BRACE =
		"expected '{' after server";
	static const char* const EXPECTED_SERVER_CLOSE_BRACE =
		"expected '}' after server block";
	static const char* const UNKNOWN_SERVER_DIRECTIVE =
		"unknown server directive";
	static const char* const EXPECTED_LISTEN_VALUE =
		"expected listen value";
	static const char* const EXPECTED_DIRECTIVE_SEMICOLON =
		"expected ';' after directive";
	static const char* const INVALID_LISTEN_VALUE =
		"invalid listen value";
	static const char* const INVALID_LISTEN_PORT =
		"invalid listen port";
}

#endif
