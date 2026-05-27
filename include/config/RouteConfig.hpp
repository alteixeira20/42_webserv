#ifndef ROUTE_CONFIG_HPP
# define ROUTE_CONFIG_HPP

# include <map>
# include <string>
# include <vector>
# include "http/HttpMethod.hpp"

class   RouteConfig
{
    public:
        typedef std::map<std::string, std::string>  CgiMap;

        RouteConfig();
        RouteConfig(const RouteConfig &other);
        RouteConfig&    operator=(const RouteConfig &other);
        ~RouteConfig();

        const std::string&  getPath() const;
        const std::vector<HttpMethod>&  getAllowedMethods() const;
        const std::string&              getRoot() const;
        const std::string&              getIndex() const;
        bool                            getAutoIndex() const;
        bool                            hasRoot() const;
        bool                            hasIndex() const;
        bool                            hasAutoIndex() const;
        bool                            hasRedirect() const;
        unsigned int                    getRedirectStatus() const;
        const std::string&              getRedirectTarget() const;
        bool                            hasUploadDir() const;
        const std::string&              getUploadDir() const;
        const std::map<std::string, std::string>&   getCgiMap() const;

        void                            setPath(const std::string &path);
        void                            addAllowedMethod(HttpMethod method);
        void                            setRoot(const std::string &root);
        void                            setIndex(const std::string &index);
        void                            setAutoIndex(bool autoindex);
        void                            setRedirect(unsigned int status, const std::string &target);
        void                            setUploadDir(const std::string &path);
        void                            addCgiHandler(const std::string &extension, const std::string &executable);

    private:
        std::string             _path;
        std::vector<HttpMethod> _allowedMethods;
        std::string             _root;
        std::string             _index;
        bool                    _autoindex;
        bool                    _hasRoot;
        bool                    _hasIndex;
        bool                    _hasAutoIndex;
        bool                    _hasRedirect;
        unsigned int            _redirectStatus;
        std::string             _redirectTarget;
        bool                    _hasUploadDir;
        std::string             _uploadDir;
        CgiMap                  _cgiMap;
};

#endif
