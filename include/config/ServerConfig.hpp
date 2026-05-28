#ifndef SERVER_CONFIG_HPP
# define SERVER_CONFIG_HPP

# include <cstddef>
# include <map>
# include <string>
# include <vector>
# include "config/ListenConfig.hpp"
# include "config/RouteConfig.hpp"

class ServerConfig
{
    public:
        typedef std::map<unsigned int, std::string> ErrorPageMap;

        ServerConfig();
        ServerConfig(const ServerConfig &other);
        ServerConfig&   operator=(const ServerConfig &other);
        ~ServerConfig();

        const std::vector<ListenConfig>&    getListens() const;
        const std::vector<std::string>&     getServerNames() const;
        const std::string&                  getRoot() const;
        const std::string&                  getIndex() const;
        std::size_t                         getClientMaxBodySize() const;
        const ErrorPageMap&                 getErrorPages() const;
        const std::vector<RouteConfig>&     getRoutes() const;

        void                                addListen(const ListenConfig &listen);
        void                                addServerName(const std::string &name);
        void                                setRoot(const std::string &root);
        void                                setIndex(const std::string &index);
        void                                setClientMaxBodySize(std::size_t size);
        void                                addErrorPage(unsigned int status, const std::string &path);
        void                                addRoute(const RouteConfig &route);

    private:
        std::vector<ListenConfig>           _listens;
		std::vector<std::string>		    _serverNames;
		std::string						    _root;
		std::string						    _index;
		std::size_t						    _clientMaxBodySize;
		ErrorPageMap					    _errorPages;
		std::vector<RouteConfig>			_routes;
};

#endif
