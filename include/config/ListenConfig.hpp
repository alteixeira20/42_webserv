#ifndef LISTEN_CONFIG_HPP
# define LISTEN_CONFIG_HPP

# include <string>

class ListenConfig
{
    public:
        ListenConfig();
        ListenConfig(const std::string &host, unsigned int port);
        ListenConfig(const ListenConfig &other);
        ListenConfig&   operator=(const ListenConfig &other);
        ~ListenConfig();

        const std::string   &getHost() const;
        unsigned int        getPort() const;
        bool                equals(const ListenConfig &other) const;

        void                setHost(const std::string &host);
        void                setPort(unsigned int port);

    private:
        std::string     _host;
        unsigned int    _port;
};

#endif
