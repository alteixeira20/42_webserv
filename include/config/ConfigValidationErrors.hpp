#ifndef CONFIG_VALIDATION_ERRORS_HPP
# define CONFIG_VALIDATION_ERRORS_HPP

/*
** Centralized messages for semantic config validation errors.
*/
namespace ConfigValidationErrors
{
	static const char* const EMPTY_CONFIG =
		"configuration must contain at least one server block";
	static const char* const SERVER_WITHOUT_LISTEN =
		"server block must contain at least one listen directive";
	static const char* const DUPLICATE_LISTEN_IN_SERVER =
		"duplicate listen endpoint inside server block";
	static const char* const DUPLICATE_DEFAULT_SERVER =
		"duplicate default server for listen endpoint";
	static const char* const DUPLICATE_SERVER_NAME =
		"duplicate server_name for listen endpoint";
	static const char* const INVALID_ROUTE_PATH =
		"location path must start with '/'";
	static const char* const DUPLICATE_ROUTE_PATH =
		"duplicate location path inside server block";
}

#endif
