#ifndef CONFIG_TOKEN_HPP
# define CONFIG_TOKEN_HPP

#include <string>

enum	ConfigTokenType
{
	CONFIG_TOKEN_WORD,
	CONFIG_TOKEN_OPEN_BRACE,
	CONFIG_TOKEN_CLOSE_BRACE,
	CONFIG_TOKEN_SEMICOLON,
	CONFIG_TOKEN_END
};

class	ConfigToken
{
	public:
		ConfigToken();
		ConfigToken(ConfigTokenType type, const std::string &value,
			unsigned int line, unsigned int column);
		ConfigToken(const ConfigToken &other);
		ConfigToken &operator=(const ConfigToken &other);
		~ConfigToken();

		ConfigTokenType		getType() const;
		const std::string&	getValue() const;
		unsigned int		getLine() const;
		unsigned int		getColumn() const;

	private:
		ConfigTokenType	_type;
		std::string		_value;
		unsigned int	_line;
		unsigned int	_column;
};

#endif