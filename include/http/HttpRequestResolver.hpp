#ifndef HTTP_REQUEST_RESOLVER_HPP
# define HTTP_REQUEST_RESOLVER_HPP

#include <string>
#include "config/Config.hpp"
#include "config/ConfigResolver.hpp"
#include "config/ListenConfig.hpp"
#include "config/RouteConfig.hpp"
#include "config/ServerConfig.hpp"
#include "http/HttpRequest.hpp"

/*
** HttpRequestResolver maps a parsed HttpRequest to the effective server and
** route from the running configuration.
**
** It strips the port from the Host header before server name matching, and
** extracts the path component from the request target before route matching.
** It rejects paths with traversal segments before any config lookup.
**
** Result::valid is true when the request path passed resolver-level validation.
** server and route may still be NULL when no config entry matches the request.
** Pointers in Result point into the Config passed to resolve(). They remain
** valid only as long as that Config object lives.
**
** This component does not open files, stat paths, or generate responses.
*/
class HttpRequestResolver
{
	public:
		struct Result
		{
			const ServerConfig	*server;
			const RouteConfig	*route;
			std::string			requestPath;
			std::string			filesystemPath; /* candidate path only — not stat'd or opened */
			bool				valid;
			unsigned int		errorStatus;
		};

		HttpRequestResolver();
		HttpRequestResolver(const HttpRequestResolver &other);
		HttpRequestResolver	&operator=(const HttpRequestResolver &other);
		~HttpRequestResolver();

		Result	resolve(const Config &config,
					const ListenConfig &listen,
					const HttpRequest &request) const;

	private:
		ConfigResolver	_resolver;

		std::string		extractHost(const std::string &hostHeader) const;
		std::string		extractPath(const std::string &target) const;
		bool			hasTraversalSegment(const std::string &path) const;
		std::string		buildFilesystemPath(const RouteConfig &route,
							const std::string &requestPath) const;
};

#endif
