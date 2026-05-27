#include "config/ConfigParser.hpp"
#include "config/ConfigException.hpp"
#include "config/ConfigTokenizer.hpp"
#include <fstream>
#include <sstream>

/*
** File reading is kept outside ConfigParser because it is only an input
** source detail. The parser itself works from strings and tokens.
*/
static std::string  readFileContent(const std::string &path)
{
    std::ifstream       file;
    std::stringstream   buffer;

    file.open(path.c_str());

    if (!file.is_open())
        throw (ConfigException("could not open config file", 0, 0));
    buffer << file.rdbuf();

    if (file.bad())
        throw (ConfigException("could not read config file", 0, 0));

    return (buffer.str());
}

ConfigParser::ConfigParser()
	: _tokens(),
	  _index(0)
{
}

ConfigParser::ConfigParser(const ConfigParser &other)
	: _tokens(other._tokens),
	  _index(other._index)
{
}

ConfigParser&	ConfigParser::operator=(const ConfigParser &other)
{
	if (this != &other)
	{
		_tokens = other._tokens;
		_index = other._index;
	}

	return (*this);
}

ConfigParser::~ConfigParser()
{
}

Config  ConfigParser::parseFile(const std::string &path)
{
	std::string	content;

	content = readFileContent(path);

	return (parseString(content));
}

Config  ConfigParser::parseString(const std::string &content)
{
    ConfigTokenizer tokenizer;
    Config          config;

    reset(tokenizer.tokenize(content));
    (void)config;

    return (config);
}

void    ConfigParser::reset(const std::vector<ConfigToken> &tokens)
{
    _tokens = tokens;
    _index = 0;
}

bool    ConfigParser::isAtEnd() const
{
    return (current().getType() == CONFIG_TOKEN_END);
}

const ConfigToken&  ConfigParser::current() const
{
    return (_tokens[_index]);
}

const ConfigToken&	ConfigParser::advance()
{
	const ConfigToken&	token = current();

	if (!isAtEnd())
		++_index;

	return (token);
}

bool	ConfigParser::check(ConfigTokenType type) const
{
	return (current().getType() == type);
}

bool	ConfigParser::match(ConfigTokenType type)
{
	if (!check(type))
		return (false);
	advance();

	return (true);
}

const ConfigToken&	ConfigParser::expect(ConfigTokenType type,
	const std::string &message)
{
	if (!check(type))
		throw(ConfigException(message, current().getLine(),
			current().getColumn()));

	return (advance());
}

const ConfigToken&	ConfigParser::expectWord(const std::string &message)
{
	return (expect(CONFIG_TOKEN_WORD, message));
}