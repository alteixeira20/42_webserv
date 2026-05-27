#include "config/ConfigException.hpp"

ConfigException::ConfigException()
    : _message("config error"),
      _line(0),
      _column(0)
{
}

ConfigException::ConfigException(const std::string &message,
    unsigned int line, unsigned int column)
    : _message(message),
      _line(line),
      _column(column)
{
}

ConfigException::ConfigException(const ConfigException &other)
    : std::exception(other),
      _message(other._message),
      _line(other._line),
      _column(other._column)
{
}

ConfigException&    ConfigException::operator=(const ConfigException &other)
{
	if (this != &other)
	{
		_message = other._message;
		_line = other._line;
		_column = other._column;
	}
	return (*this); 
}

ConfigException::~ConfigException() throw()
{
}

const char* ConfigException::what() const throw()
{
    return (_message.c_str());
}

unsigned int    ConfigException::getLine() const
{
	return (_line);
}

unsigned int    ConfigException::getColumn() const
{
    return (_column);    
}