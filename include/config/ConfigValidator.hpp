#ifndef CONFIG_VALIDATOR_HPP
# define CONFIG_VALIDATOR_HPP

# include <map>
# include <set>
# include <string>
# include "config/Config.hpp"
# include "config/ListenConfig.hpp"
# include "config/RouteConfig.hpp"
# include "config/ServerConfig.hpp"

/*
** ConfigValidator checks semantic correctness after parsing.
**
** The parser owns syntax. The validator owns cross-field rules such as
** required listen endpoints, duplicate endpoints, and duplicate routes.
*/

class ConfigValidator
{
	public:
		ConfigValidator();
		ConfigValidator(const ConfigValidator &other);
		ConfigValidator&	operator=(const ConfigValidator &other);
		~ConfigValidator();

		void				validate(const Config &config) const;

	private:
		typedef std::set<std::string>				StringSet;
		typedef std::map<std::string, StringSet>	VirtualHostMap;

		void				validateServer(const ServerConfig &server,
								VirtualHostMap &virtualHosts) const;
		void				validateServerListens(
								const ServerConfig &server) const;
		void				validateVirtualHosts(
								const ServerConfig &server,
								VirtualHostMap &virtualHosts) const;
		void				validateVirtualHostName(
								const std::string &listenKey,
								const std::string &serverName,
								VirtualHostMap &virtualHosts) const;
		void				validateRoutes(const ServerConfig &server) const;
		void				validateRoute(const RouteConfig &route,
								StringSet &routePaths) const;
		std::string			listenKey(const ListenConfig &listen) const;
};

#endif
