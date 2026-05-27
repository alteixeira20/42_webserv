#include "ListenerConfig.hpp"

ListenerConfig::ListenerConfig(void) :
	host("0.0.0.0"),
	port(8080)
{
}

ListenerConfig::ListenerConfig(const std::string &host, unsigned short port) :
	host(host),
	port(port)
{
}
