#include "config/ServerConfig.hpp"

ServerConfig::ServerConfig()
    : _listens(),
      _serverNames(),
      _root(""),
      _index("index.html"),
      _clientMaxBodySize(1000000),
      _errorPages(),
      _routes()
{
}

ServerConfig::ServerConfig(const ServerConfig &other)
	: _listens(other._listens),
	  _serverNames(other._serverNames),
	  _root(other._root),
	  _index(other._index),
	  _clientMaxBodySize(other._clientMaxBodySize),
	  _errorPages(other._errorPages),
	  _routes(other._routes)
{
}

ServerConfig&	ServerConfig::operator=(const ServerConfig &other)
{
	if (this != &other)
	{
		_listens = other._listens;
		_serverNames = other._serverNames;
		_root = other._root;
		_index = other._index;
		_clientMaxBodySize = other._clientMaxBodySize;
		_errorPages = other._errorPages;
		_routes = other._routes;
	}
	return (*this);
}

ServerConfig::~ServerConfig()
{
}

const std::vector<ListenConfig>&    ServerConfig::getListens() const
{
    return (_listens);
}

const std::vector<std::string>& ServerConfig::getServerNames() const
{
    return (_serverNames);
}

const std::string&  ServerConfig::getRoot() const
{
    return (_root);
}

const std::string&  ServerConfig::getIndex() const
{
	return (_index);
}

std::size_t	ServerConfig::getClientMaxBodySize() const
{
	return (_clientMaxBodySize);
}

const ServerConfig::ErrorPageMap&	ServerConfig::getErrorPages() const
{
	return (_errorPages);
}

const std::vector<RouteConfig>&	ServerConfig::getRoutes() const
{
	return (_routes);
}

void    ServerConfig::addListen(const ListenConfig &listen)
{
    std::vector<ListenConfig>::const_iterator   it;

    it = _listens.begin();

    while (it != _listens.end())
    {
        if (it->equals(listen))
            return ;
        ++it;
    }
    _listens.push_back(listen);
}

void    ServerConfig::addServerName(const std::string &name)
{
    std::vector<std::string>::const_iterator   it;

    it = _serverNames.begin();

    while (it != _serverNames.end())
    {
        if (*it == name)
            return ;
        ++it;
    }
    _serverNames.push_back(name);
}

void	ServerConfig::setRoot(const std::string &root)
{
	_root = root;
}

void	ServerConfig::setIndex(const std::string &index)
{
	_index = index;
}

void	ServerConfig::setClientMaxBodySize(std::size_t size)
{
	_clientMaxBodySize = size;
}

void	ServerConfig::addErrorPage(unsigned int status,
	const std::string &path)
{
	_errorPages[status] = path;
}

void	ServerConfig::addRoute(const RouteConfig &route)
{
	_routes.push_back(route);
}