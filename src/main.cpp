#include <iostream>

int	main(int argc, char **argv)
{
	std::string config = (argc > 1) ? argv[1] : "configs/default.conf";
	std::cout << "webserv starting with config: " << config << std::endl;
	return (0);
}
