#include "config/Config.hpp"
#include "config/ConfigParser.hpp"
#include "config/ListenConfig.hpp"
#include "runtime/ServerRuntime.hpp"

#include <ctime>
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
		ServerRuntime				runtime;
		std::vector<ListenConfig>	endpoints;
		const int					pollTimeoutMs = 1000;
		const int					clientTimeoutSeconds = 30;

		configData = parser.parseFile(configPath);
		endpoints = configData.getUniqueListens();
		runtime.start(endpoints);
		std::cout << "listening on " << runtime.listenerCount()
			<< " endpoint(s)" << std::endl;
		while (true)
		{
			runtime.runCycle(pollTimeoutMs);
			runtime.cleanup(std::time(NULL), clientTimeoutSeconds);
		}
	}
	catch (const std::exception &error)
	{
		std::cerr << "webserv: " << error.what() << std::endl;
		return (1);
	}
	return (0);
}
