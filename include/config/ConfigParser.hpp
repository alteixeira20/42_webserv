#ifndef CONFIG_PARSER_HPP
# define CONFIG_PARSER_HPP

#include <cstddef>
#include <string>
#include <vector>
#include "config/Config.hpp"
#include "config/ConfigToken.hpp"
#include "config/ServerConfig.hpp"

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
		void					    reset(const std::vector<ConfigToken> &tokens);
		bool				    	isAtEnd() const;
		const ConfigToken&		    current() const;
		const ConfigToken&			advance();

		/* Token matching */
		bool					check(ConfigTokenType type) const;
		bool					match(ConfigTokenType type);
		const ConfigToken&		expect(ConfigTokenType type,
			const std::string &message);
		const ConfigToken&		expectWord(const std::string &message);

		/* Grammar parsing */
		Config					parseConfig();
		ServerConfig			parseServer();
		void					parseServerDirective(ServerConfig &server);
};

#endif
