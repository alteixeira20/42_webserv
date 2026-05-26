#include "config/ConfigTokenizer.hpp"

ConfigTokenizer::ConfigTokenizer()
    : _content(""),
      _index(0),
      _line(1),
      _column(1)
{
}

ConfigTokenizer::ConfigTokenizer(const ConfigTokenizer &other)
    : _content(other._content),
      _index(other._index),
      _line(other._line),
      _column(other._column)
{
}

ConfigTokenizer&    ConfigTokenizer::operator=(const ConfigTokenizer &other)
{   
    if (this != &other)
    {
        _content = other._content;
        _index = other._index;
        _line = other._line;
        _column = other._column;
    }
    return (*this);
}

ConfigTokenizer::~ConfigTokenizer()
{    
}

std::vector<ConfigToken>    ConfigTokenizer::tokenize(
    const std::string &content)
{
    std::vector<ConfigToken>    tokens;
    char                        current;

    _content = content;
    _index = 0;
    _line = 1;
    _column = 1;

    while (!isAtEnd())
    {
        skipWhitespaceAndComments();
        if (isAtEnd())
            break ;
        current = peek();
        if (current == '{')
            tokens.push_back(makeToken(CONFIG_TOKEN_OPEN_BRACE, "{"));
        else if (current == '}')
            tokens.push_back(makeToken(CONFIG_TOKEN_CLOSE_BRACE, "}"));
        else if (current == ';')
            tokens.push_back(makeToken(CONFIG_TOKEN_SEMICOLON, ";"));
        else
            tokens.push_back(readWord());
        if (current == '{' || current == '}' || current == ';')
            advance();
    }
    tokens.push_back(makeToken(CONFIG_TOKEN_END, ""));
    return (tokens);
}

bool    ConfigTokenizer::isAtEnd() const
{
    return (_index >= _content.length());
}

char    ConfigTokenizer::peek() const
{
    if (isAtEnd())
        return ('\0');
    return (_content[_index]);
}

char    ConfigTokenizer::advance()
{
    char    current;
    current = peek();

    if (isAtEnd())
        return ('\0');
    ++_index;
    if (current == '\n')
    {
        ++_line;
        _column = 1;
    }
    else
        ++_column;
    return (current);
}

void   ConfigTokenizer::skipWhitespaceAndComments()
{
    while (!isAtEnd())
    {
        if (peek() == '#')
        {
            while (!isAtEnd() && peek() != '\n')
                advance();
        }
        else if (peek() == ' ' || peek() == '\t'
            || peek() == '\n' || peek() == '\r')
            advance();
        else
            return ;
    }
}

ConfigToken ConfigTokenizer::readWord()
{
    std::string     value;
    unsigned int    line;
    unsigned int    column;

    line = _line;
    column = _column;
    while (!isAtEnd() && peek() != '{' && peek() != '}'
        && peek() != ';' && peek() != '#' && peek() != ' '
        && peek() != '\t' && peek() != '\n' && peek() != '\r')
        value += advance();
    return (ConfigToken(CONFIG_TOKEN_WORD, value, line, column));
}

ConfigToken ConfigTokenizer::makeToken(ConfigTokenType type,
    const std::string &value) const
{
    return (ConfigToken(type, value, _line, _column));
}
