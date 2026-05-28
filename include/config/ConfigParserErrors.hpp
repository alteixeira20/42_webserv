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
	static const char* const EXPECTED_SERVER_NAME =
		"expected server name";
	static const char* const EXPECTED_ROOT_VALUE =
		"expected root value";
	static const char* const EXPECTED_INDEX_VALUE =
		"expected index value";
	static const char* const EXPECTED_BODY_SIZE_VALUE =
		"expected client_max_body_size value";
	static const char* const INVALID_BODY_SIZE =
		"invalid client_max_body_size value";
	static const char* const EXPECTED_ERROR_STATUS =
		"expected error_page status code";
	static const char* const EXPECTED_ERROR_PAGE_PATH =
		"expected error_page path";
	static const char* const INVALID_ERROR_STATUS =
		"invalid error_page status code";
	static const char* const EXPECTED_LOCATION_PATH =
		"expected location path";
	static const char* const EXPECTED_LOCATION_OPEN_BRACE =
		"expected '{' after location path";
	static const char* const EXPECTED_LOCATION_CLOSE_BRACE =
		"expected '}' after location block";
	static const char* const UNKNOWN_ROUTE_DIRECTIVE =
		"unknown route directive";
	static const char* const EXPECTED_ROUTE_ROOT_VALUE =
		"expected route root value";
	static const char* const EXPECTED_ROUTE_INDEX_VALUE =
		"expected route index value";
	static const char* const EXPECTED_AUTOINDEX_VALUE =
		"expected autoindex value";
	static const char* const INVALID_BOOLEAN_VALUE =
		"invalid boolean value";
	static const char* const EXPECTED_ALLOWED_METHOD =
		"expected allowed method";
	static const char* const INVALID_ALLOWED_METHOD =
		"invalid allowed method";
	static const char* const EXPECTED_REDIRECT_STATUS =
		"expected redirect status";
	static const char* const EXPECTED_REDIRECT_TARGET =
		"expected redirect target";
	static const char* const INVALID_REDIRECT_STATUS =
		"invalid redirect status";
	static const char* const EXPECTED_UPLOAD_DIR =
		"expected upload directory";
	static const char* const EXPECTED_CGI_EXTENSION =
		"expected CGI extension";
	static const char* const EXPECTED_CGI_EXECUTABLE =
		"expected CGI executable";
}

#endif
