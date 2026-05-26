#include "config/ConfigToken.hpp"

ConfigToken::ConfigToken()
    : _type(CONFIG_TOKEN_END),
      _value(""),
      _line(0),
      _column(0)
{
}

ConfigToken::ConfigToken(ConfigTokenType type, const std::string &value,
    unsigned int line, unsigned int column)
    : _type(type),
      _value(value),
      _line(line),
      _column(column)
{
}

ConfigToken::ConfigToken(const ConfigToken &other)
    : _type(other._type),
      _value(other._value),
      _line(other._line),
      _column(other._column)
{
}

ConfigToken &ConfigToken::operator=(const ConfigToken &other)
{
    if (this != &other)
    {
        _type = other._type;
        _value = other._value;
        _line = other._line;
        _column = other._column;
    }
    return (*this);
}

ConfigToken::~ConfigToken()
{
}

ConfigTokenType ConfigToken::getType() const
{
    return (_type);
}

const std::string &ConfigToken::getValue() const
{
    return (_value);
}

unsigned int ConfigToken::getLine() const
{
    return (_line);
}

unsigned int ConfigToken::getColumn() const
{
    return (_column);
}