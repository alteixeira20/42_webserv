#ifndef CONFIG_RESOLVER_HPP
# define CONFIG_RESOLVER_HPP

# include <string>
# include "config/Config.hpp"
# include "config/ListenConfig.hpp"
# include "config/RouteConfig.hpp"
# include "config/ServerConfig.hpp"

/*
** ConfigResolver selects the effective server and route for a request.
**
** It does not parse or validate configuration. It only applies runtime
** selection rules to an already validated Config object.
*/

class	ConfigResolver
{
	public:
		ConfigResolver();
		ConfigResolver(const ConfigResolver &other);
		ConfigResolver&	operator=(const ConfigResolver &other);
		~ConfigResolver();

		const ServerConfig*	findServer(const Config &config,
								const ListenConfig &listen,
								const std::string &host) const;
		const RouteConfig*	findRoute(const ServerConfig &server,
								const std::string &path) const;

	private:
		bool				serverListensOn(const ServerConfig &server,
								const ListenConfig &listen) const;
		bool				listenMatches(const ListenConfig &left,
								const ListenConfig &right) const;
		bool				serverNameMatches(const ServerConfig &server,
								const std::string &host) const;
		bool				routeMatchesPath(const std::string &routePath,
								const std::string &requestPath) const;
};

#endif
