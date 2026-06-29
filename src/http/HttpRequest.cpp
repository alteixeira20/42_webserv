#include "http/HttpRequest.hpp"

HttpRequest::HttpRequest()
	: _method(HTTP_METHOD_UNKNOWN),
	  _target(),
	  _version(),
	  _headers(),
	  _body()
{
}

HttpRequest::HttpRequest(const HttpRequest &other)
	: _method(other._method),
	  _target(other._target),
	  _version(other._version),
	  _headers(other._headers),
	  _body(other._body)
{
}

HttpRequest	&HttpRequest::operator=(const HttpRequest &other)
{
	if (this != &other)
	{
		_method = other._method;
		_target = other._target;
		_version = other._version;
		_headers = other._headers;
		_body = other._body;
	}

	return (*this);
}

HttpRequest::~HttpRequest()
{
}

void	HttpRequest::clear()
{
	_method = HTTP_METHOD_UNKNOWN;
	_target.clear();
	_version.clear();
	_headers.clear();
	_body.clear();
}

void	HttpRequest::setMethod(HttpMethod method)
{
	_method = method;
}

void	HttpRequest::setTarget(const std::string &target)
{
	_target = target;
}

void	HttpRequest::setVersion(const std::string &version)
{
	_version = version;
}

void	HttpRequest::setHeader(const std::string &name,
	const std::string &value)
{
	_headers[normalizeHeaderName(name)] = value;
}

void	HttpRequest::setBody(const std::string &body)
{
	_body = body;
}

HttpMethod	HttpRequest::getMethod() const
{
	return (_method);
}

const std::string	&HttpRequest::getTarget() const
{
	return (_target);
}

const std::string	&HttpRequest::getVersion() const
{
	return (_version);
}

const HttpRequest::HeaderMap	&HttpRequest::getHeaders() const
{
	return (_headers);
}

const std::string	&HttpRequest::getBody() const
{
	return (_body);
}

std::size_t	HttpRequest::bodySize() const
{
	return (_body.size());
}

bool	HttpRequest::hasHeader(const std::string &name) const
{
	return (_headers.find(normalizeHeaderName(name)) != _headers.end());
}

std::string	HttpRequest::getHeader(const std::string &name) const
{
	HeaderMap::const_iterator	it;

	it = _headers.find(normalizeHeaderName(name));
	if (it == _headers.end())
		return ("");

	return (it->second);
}

std::string	HttpRequest::normalizeHeaderName(const std::string &name) const
{
	std::string				normalized;
	std::string::size_type	index;

	normalized = name;
	index = 0;
	while (index < normalized.length())
	{
		if (normalized[index] >= 'A' && normalized[index] <= 'Z')
			normalized[index] = normalized[index] - 'A' + 'a';
		++index;
	}

	return (normalized);
}
