#include "config/Config.hpp"
#include "config/ConfigParser.hpp"
#include "config/ListenConfig.hpp"
#include "runtime/ListenerManager.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <vector>

int	main(int argc, char **argv)
{
	std::string	configPath = (argc > 1) ? argv[1] : "configs/default.conf";

	std::cout << "webserv starting with config: " << configPath << std::endl;
	try
	{
		ConfigParser				parser;
		Config						configData;
		ListenerManager				listeners;
		std::vector<ListenConfig>	endpoints;

		configData = parser.parseFile(configPath);
		endpoints = configData.getUniqueListens();
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
