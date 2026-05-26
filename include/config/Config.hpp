#ifndef CONFIG_HPP
# define CONFIG_HPP

#include <vector>
#include "config/ListenConfig.hpp"
#include "config/ServerConfig.hpp"

class   Config
{
	public:
		Config(void);
		Config(const Config& other);
		Config&	operator=(const Config& other);
		~Config(void);
        
        void                                addServer(const ServerConfig &server);
        const std::vector<ServerConfig>&    getServers() const;
        std::vector<ListenConfig>           getUniqueListens() const;

    private:
        std::vector<ServerConfig>           _servers;
};

#endif
