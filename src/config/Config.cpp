#include "config/Config.hpp"

static bool containsListen(const std::vector<ListenConfig> &listens,
    const ListenConfig &target)
{
    std::vector<ListenConfig>::const_iterator   it;

    it = listens.begin();
    while (it != listens.end())
    {
        if (it->equals(target))
            return (true);
        ++it;
    }
    return (false);
}

    Config::Config(void)
	: _servers()
{
}

Config::Config(const Config& other)
	: _servers(other._servers)
{
}

Config&	Config::operator=(const Config& other)
{
	if (this != &other)
		_servers = other._servers;
	return (*this);
}

Config::~Config(void)
{
}

void	Config::addServer(const ServerConfig& server)
{
	_servers.push_back(server);
}

const std::vector<ServerConfig>&	Config::getServers(void) const
{
	return (_servers);
}

std::vector<ListenConfig>   Config::getUniqueListens() const
{
    std::vector<ListenConfig>                   result;
    std::vector<ServerConfig>::const_iterator   serverIt;
    std::vector<ListenConfig>::const_iterator   listenIt;

    serverIt = _servers.begin();

    while (serverIt != _servers.end())
    {
        listenIt = serverIt->getListens().begin();
        while (listenIt != serverIt->getListens().end())
        {
            if (!containsListen(result, *listenIt))
                result.push_back(*listenIt);
            ++listenIt;
        }
        ++serverIt;
    }
    return (result);
}