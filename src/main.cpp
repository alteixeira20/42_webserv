#include "ListenerConfig.hpp"
#include "ListenerManager.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <vector>

static std::vector<ListenerConfig>	temporary_listener_configs(void)
{
	std::vector<ListenerConfig> configs;

	configs.push_back(ListenerConfig("0.0.0.0", 8080));
	return (configs);
}

int	main(int argc, char **argv)
{
	std::string config = (argc > 1) ? argv[1] : "configs/default.conf";

	std::cout << "webserv starting with config: " << config << std::endl;
	try
	{
		ListenerManager listeners;
		std::vector<ListenerConfig> endpoints = temporary_listener_configs();

		listeners.openAll(endpoints);
		std::cout << "listening on " << listeners.listeners().size()
			<< " endpoint(s)" << std::endl;
	}
	catch (const std::exception &error)
	{
		std::cerr << "webserv: " << error.what() << std::endl;
		return (1);
	}
	return (0);
}
