#ifndef HTTP_METHOD_HPP
# define HTTP_METHOD_HPP

#include <string>
#include <vector>

enum	HttpMethod
{
	HTTP_METHOD_UNKNOWN = 0,
	HTTP_METHOD_GET,
	HTTP_METHOD_POST,
	HTTP_METHOD_DELETE
};

HttpMethod	parseHttpMethod(const std::string &value);
std::string	httpMethodToStr(HttpMethod method);
bool		isSupportedHttpMethod(HttpMethod method);
bool		containsHttpMethod(const std::vector<HttpMethod> &methods, HttpMethod method);

#endif
