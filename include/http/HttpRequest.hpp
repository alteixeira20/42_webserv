#ifndef HTTP_REQUEST_HPP
# define HTTP_REQUEST_HPP

# include <map>
# include <string>
# include "http/HttpMethod.hpp"

/*
** HttpRequest stores the structured result of HTTP request parsing.
**
** It owns protocol fields only. It does not read sockets, resolve routes,
** access the filesystem, or build responses.
*/

class HttpRequest
{
	public:
		typedef std::map<std::string, std::string>	HeaderMap;

		HttpRequest();
		HttpRequest(const HttpRequest &other);
		HttpRequest	&operator=(const HttpRequest &other);
		~HttpRequest();

		void				clear();

		void				setMethod(HttpMethod method);
		void				setTarget(const std::string &target);
		void				setVersion(const std::string &version);
		void				setHeader(const std::string &name,
								const std::string &value);

		HttpMethod			getMethod() const;
		const std::string	&getTarget() const;
		const std::string	&getVersion() const;
		const HeaderMap		&getHeaders() const;

		bool				hasHeader(const std::string &name) const;
		std::string			getHeader(const std::string &name) const;

	private:
		HttpMethod			_method;
		std::string			_target;
		std::string			_version;
		HeaderMap			_headers;

		std::string			normalizeHeaderName(
								const std::string &name) const;
};

#endif
