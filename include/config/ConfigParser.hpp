#ifndef CONFIG_PARSER_HPP
# define CONFIG_PARSER_HPP

#include <cstddef>
#include <string>
#include <vector>
#include "config/Config.hpp"
#include "config/ConfigToken.hpp"
#include "config/ListenConfig.hpp"
#include "config/RouteConfig.hpp"
#include "config/ServerConfig.hpp"
#include "http/HttpMethod.hpp"

/*
** ConfigParser turns tokenized configuration text into Config objects.
** It owns parser state while parsing, but does not perform runtime actions.
*/
class ConfigParser
{
	public:
		ConfigParser();
		ConfigParser(const ConfigParser &other);
		ConfigParser&	operator=(const ConfigParser &other);
		~ConfigParser(void);

        Config     parseFile(const std::string &path);
        Config     parseString(const std::string &content);

    private:
        std::vector<ConfigToken>	_tokens;
		std::size_t				    _index;

		/* Parser state */
		void					reset(const std::vector<ConfigToken> &tokens);
		bool				    isAtEnd() const;
		const ConfigToken&		current() const;
		const ConfigToken&		advance();

		/*
		** Token matching.
		** expect() is the central place for syntax error locations.
		*/
		bool					check(ConfigTokenType type) const;
		bool					match(ConfigTokenType type);
		const ConfigToken&		expect(ConfigTokenType type,
			const std::string &message);
		const ConfigToken&		expectWord(const std::string &message);

		/*
		** Server grammar.
		** These functions parse server-level structure and directives.
		*/
		Config					parseConfig();
		ServerConfig			parseServer();
		void					parseServerDirective(ServerConfig &server);
		void					parseListen(ServerConfig &server);
		void					parseServerName(ServerConfig &server);
		void					parseRoot(ServerConfig &server);
		void					parseIndex(ServerConfig &server);
		void					parseClientMaxBodySize(ServerConfig &server);
		void					parseErrorPage(ServerConfig &server);
		void					addErrorPages(ServerConfig &server,
									const std::vector<unsigned int> &statuses,
									const std::string &path);

		/*
		** Value parsing.
		** These helpers convert directive values into typed config data.
		*/
		ListenConfig			parseListenValue(
									const ConfigToken &token) const;
		unsigned int			parsePort(const std::string &value,
									const ConfigToken &token) const;
		std::size_t				parseSize(const std::string &value,
									const ConfigToken &token) const;
		unsigned int			parseStatusCode(const std::string &value,
									const ConfigToken &token) const;
		bool					isOnlyDigits(const std::string &value) const;
		bool					parseBoolean(const std::string &value,
									const ConfigToken &token) const;
		HttpMethod				parseAllowedMethod(const ConfigToken &token) const;
		unsigned int			parseRedirectStatus(const std::string &value,
									const ConfigToken &token) const;

		/*
		** Route grammar.
		** These functions parse location blocks inside a server block.
		*/
		void					parseLocation(ServerConfig &server);
		RouteConfig				parseRoute();
		void					parseRouteDirective(RouteConfig &route);
		void					parseRouteRoot(RouteConfig &route);
		void					parseRouteIndex(RouteConfig &route);
		void					parseAutoIndex(RouteConfig &route);
		void					parseAllowedMethods(RouteConfig &route);
		void					parseRedirect(RouteConfig &route);
		void					parseUploadDir(RouteConfig &route);
		void					parseCgi(RouteConfig &route);
};

#endif
