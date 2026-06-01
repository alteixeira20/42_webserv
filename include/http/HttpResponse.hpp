#ifndef HTTP_RESPONSE_HPP
# define HTTP_RESPONSE_HPP

# include <cstddef>
# include <map>
# include <string>

class HttpResponse
{
public:
	typedef std::map<std::string, std::string>	HeaderMap;

	HttpResponse(void);

	void				setStatus(unsigned int code);
	void				setStatus(unsigned int code,
							const std::string &reason);
	void				setHeader(const std::string &name,
							const std::string &value);
	void				setBody(const std::string &body);
	void				setCloseConnection(bool closeConnection);

	unsigned int		statusCode(void) const;
	const std::string	&reasonPhrase(void) const;
	const std::string	&body(void) const;
	bool				closeConnection(void) const;
	std::string			serialize(void) const;

private:
	unsigned int	_statusCode;
	std::string		_reasonPhrase;
	HeaderMap		_headers;
	std::string		_body;
	bool			_closeConnection;

	static std::string	defaultReasonPhrase(unsigned int code);
	static std::string	sizeToString(std::size_t value);
};

#endif
