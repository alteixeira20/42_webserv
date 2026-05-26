#ifndef CONFIG_EXCEPTION_HPP
# define CONFIG_EXCEPTION_HPP

#include <exception>
#include <string>

class ConfigException: public std::exception
{

    public:
        ConfigException();
        ConfigException(const std::string &message, unsigned int line,
            unsigned int column);
        ConfigException(const ConfigException &other);
        ConfigException &operator=(const ConfigException &other);
        virtual ~ConfigException() throw();
        
        virtual const char* what() const throw();
        unsigned int        getLine() const;
        unsigned int        getColumn() const;

    private:
            std::string     _message;
            unsigned int    _line;
            unsigned int    _column;
};

#endif