#ifndef CONFIG_PARSER_HPP
# define CONFIG_PARSER_HPP

#include <cstddef>
#include <string>
#include <vector>
#include "config/Config.hpp"
#include "config/ConfigToken.hpp"

class ConfigParser
{
	public:
		ConfigParser(void);
		ConfigParser(const ConfigParser& other);
		ConfigParser&	operator=(const ConfigParser& other);
		~ConfigParser(void);

        Config     parseFile(const std::string &path);
        Config     parseString(const std::string &content);

    private:
        std::vector<ConfigToken>	_tokens;
		std::size_t				    _index;

		void					    reset(const std::vector<ConfigToken>& tokens);
		bool				    	isAtEnd(void) const;
		const ConfigToken&		    current(void) const;
};

#endif