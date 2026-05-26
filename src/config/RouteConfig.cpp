#include "config/RouteConfig.hpp"

RouteConfig::RouteConfig(void)
	: _path(""),
	  _allowedMethods(),
	  _root(""),
	  _index(""),
	  _autoindex(false),
	  _hasRoot(false),
	  _hasIndex(false),
	  _hasAutoIndex(false),
	  _hasRedirect(false),
	  _redirectStatus(0),
	  _redirectTarget(""),
	  _hasUploadDir(false),
	  _uploadDir(""),
	  _cgiMap()
{
}

RouteConfig::RouteConfig(const RouteConfig& other)
	: _path(other._path),
	  _allowedMethods(other._allowedMethods),
	  _root(other._root),
	  _index(other._index),
	  _autoindex(other._autoindex),
	  _hasRoot(other._hasRoot),
	  _hasIndex(other._hasIndex),
	  _hasAutoIndex(other._hasAutoIndex),
	  _hasRedirect(other._hasRedirect),
	  _redirectStatus(other._redirectStatus),
	  _redirectTarget(other._redirectTarget),
	  _hasUploadDir(other._hasUploadDir),
	  _uploadDir(other._uploadDir),
	  _cgiMap(other._cgiMap)
{
}

RouteConfig&	RouteConfig::operator=(const RouteConfig& other)
{
	if (this != &other)
	{
		_path = other._path;
		_allowedMethods = other._allowedMethods;
		_root = other._root;
		_index = other._index;
		_autoindex = other._autoindex;
		_hasRoot = other._hasRoot;
		_hasIndex = other._hasIndex;
		_hasAutoIndex = other._hasAutoIndex;
		_hasRedirect = other._hasRedirect;
		_redirectStatus = other._redirectStatus;
		_redirectTarget = other._redirectTarget;
		_hasUploadDir = other._hasUploadDir;
		_uploadDir = other._uploadDir;
		_cgiMap = other._cgiMap;
	}
	return (*this);
}

RouteConfig::~RouteConfig()
{
}

const std::string&  RouteConfig::getPath() const
{
    return (_path);
}

const std::vector<HttpMethod>&  RouteConfig::getAllowedMethods() const
{
	return (_allowedMethods);
}

const std::string&	RouteConfig::getRoot(void) const
{
	return (_root);
}

const std::string&	RouteConfig::getIndex(void) const
{
	return (_index);
}

bool	RouteConfig::getAutoIndex(void) const
{
	return (_autoindex);
}

bool	RouteConfig::hasRoot(void) const
{
	return (_hasRoot);
}

bool	RouteConfig::hasIndex(void) const
{
	return (_hasIndex);
}

bool	RouteConfig::hasAutoIndex(void) const
{
	return (_hasAutoIndex);
}

bool	RouteConfig::hasRedirect(void) const
{
	return (_hasRedirect);
}

unsigned int	RouteConfig::getRedirectStatus(void) const
{
	return (_redirectStatus);
}

const std::string&	RouteConfig::getRedirectTarget(void) const
{
	return (_redirectTarget);
}

bool	RouteConfig::hasUploadDir(void) const
{
	return (_hasUploadDir);
}

const std::string&	RouteConfig::getUploadDir(void) const
{
	return (_uploadDir);
}

const RouteConfig::CgiMap&	RouteConfig::getCgiMap(void) const
{
	return (_cgiMap);
}

void	RouteConfig::setPath(const std::string& path)
{
	_path = path;
}

void	RouteConfig::addAllowedMethod(HttpMethod method)
{
	if (!containsHttpMethod(_allowedMethods, method))
		_allowedMethods.push_back(method);
}

void	RouteConfig::setRoot(const std::string& root)
{
	_root = root;
	_hasRoot = true;
}

void	RouteConfig::setIndex(const std::string& index)
{
	_index = index;
	_hasIndex = true;
}

void	RouteConfig::setAutoIndex(bool autoindex)
{
	_autoindex = autoindex;
	_hasAutoIndex = true;
}

void	RouteConfig::setRedirect(unsigned int status,
	const std::string& target)
{
	_redirectStatus = status;
	_redirectTarget = target;
	_hasRedirect = true;
}

void	RouteConfig::setUploadDir(const std::string& path)
{
	_uploadDir = path;
	_hasUploadDir = true;
}

void	RouteConfig::addCgiHandler(const std::string& extension,
	const std::string& executable)
{
	_cgiMap[extension] = executable;
}