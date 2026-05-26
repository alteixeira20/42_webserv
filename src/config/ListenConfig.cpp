#include "config/ListenConfig.hpp"

ListenConfig::ListenConfig()
    : _host("0.0.0.0"),
      _port(0)
{
}

ListenConfig::ListenConfig(const std::string &host, unsigned int port)
	: _host(host),
	  _port(port)
{
}

ListenConfig::ListenConfig(const ListenConfig &other)
	: _host(other._host),
	  _port(other._port)
{
}

ListenConfig&	ListenConfig::operator=(const ListenConfig &other)
{
	if (this != &other)
	{
		_host = other._host;
		_port = other._port;
	}
	return (*this);
}

ListenConfig::~ListenConfig()
{
}

const std::string&	ListenConfig::getHost() const
{
	return (_host);
}

unsigned int	ListenConfig::getPort() const
{
	return (_port);
}

bool	ListenConfig::equals(const ListenConfig &other) const
{
	return (_host == other._host && _port == other._port);
}

void	ListenConfig::setHost(const std::string &host)
{
	_host = host;
}

void	ListenConfig::setPort(unsigned int port)
{
	_port = port;
}