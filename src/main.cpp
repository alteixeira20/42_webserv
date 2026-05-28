#include "ListenerConfig.hpp"
#include "ServerRuntime.hpp"

#include <ctime>
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
		std::vector<ListenerConfig> endpoints = temporary_listener_configs();
		ServerRuntime runtime;

		runtime.start(endpoints);
		std::cout << "listening on " << runtime.listenerCount()
			<< " endpoint(s)" << std::endl;
		while (true)
		{
			runtime.runCycle(1000);
			runtime.cleanup(std::time(NULL), 30);
		}
	}
	catch (const std::exception &error)
	{
		std::cerr << "webserv: " << error.what() << std::endl;
		return (1);
	}
	return (0);
}
