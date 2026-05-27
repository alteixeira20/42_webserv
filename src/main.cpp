#include "config/Config.hpp"
#include "config/ConfigParser.hpp"
#include "config/ListenConfig.hpp"
#include "config/ServerConfig.hpp"
#include "ListenerManager.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <vector>

static void	add_parser_foundation_default(Config &config)
{
	ServerConfig server;

	server.addListen(ListenConfig("0.0.0.0", 8080));
	config.addServer(server);
}

int	main(int argc, char **argv)
{
	std::string config = (argc > 1) ? argv[1] : "configs/default.conf";

	std::cout << "webserv starting with config: " << config << std::endl;
	try
	{
		ConfigParser parser;
		Config configData = parser.parseFile(config);
		ListenerManager listeners;
		std::vector<ListenConfig> endpoints = configData.getUniqueListens();

		if (endpoints.empty())
		{
			add_parser_foundation_default(configData);
			endpoints = configData.getUniqueListens();
		}
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
