#include "http/HttpResponse.hpp"

#include <sstream>

HttpResponse::HttpResponse(void) :
	_statusCode(200),
	_reasonPhrase("OK"),
	_headers(),
	_body(),
	_closeConnection(false)
{
}

void	HttpResponse::setStatus(unsigned int code)
{
	_statusCode = code;
	_reasonPhrase = defaultReasonPhrase(code);
}

void	HttpResponse::setStatus(unsigned int code,
	const std::string &reason)
{
	_statusCode = code;
	_reasonPhrase = reason;
}

void	HttpResponse::setHeader(const std::string &name,
	const std::string &value)
{
	_headers[name] = value;
}

void	HttpResponse::setBody(const std::string &body)
{
	_body = body;
}

void	HttpResponse::setCloseConnection(bool closeConnection)
{
	_closeConnection = closeConnection;
}

unsigned int	HttpResponse::statusCode(void) const
{
	return (_statusCode);
}

const std::string	&HttpResponse::reasonPhrase(void) const
{
	return (_reasonPhrase);
}

const std::string	&HttpResponse::body(void) const
{
	return (_body);
}

bool	HttpResponse::closeConnection(void) const
{
	return (_closeConnection);
}

std::string	HttpResponse::serialize(void) const
{
	std::ostringstream	output;
	HeaderMap			headers(_headers);

	headers["Content-Length"] = sizeToString(_body.size());
	if (_closeConnection)
		headers["Connection"] = "close";
	else
		headers["Connection"] = "keep-alive";
	output << "HTTP/1.1 " << _statusCode << " " << _reasonPhrase << "\r\n";
	for (HeaderMap::const_iterator it = headers.begin(); it != headers.end();
		++it)
		output << it->first << ": " << it->second << "\r\n";
	output << "\r\n";
	output << _body;
	return (output.str());
}

std::string	HttpResponse::defaultReasonPhrase(unsigned int code)
{
	if (code == 200)
		return ("OK");
	if (code == 201)
		return ("Created");
	if (code == 204)
		return ("No Content");
	if (code == 301)
		return ("Moved Permanently");
	if (code == 302)
		return ("Found");
	if (code == 400)
		return ("Bad Request");
	if (code == 403)
		return ("Forbidden");
	if (code == 404)
		return ("Not Found");
	if (code == 405)
		return ("Method Not Allowed");
	if (code == 413)
		return ("Payload Too Large");
	if (code == 431)
		return ("Request Header Fields Too Large");
	if (code == 500)
		return ("Internal Server Error");
	if (code == 505)
		return ("HTTP Version Not Supported");
	return ("");
}

std::string	HttpResponse::sizeToString(std::size_t value)
{
	std::ostringstream	output;

	output << value;
	return (output.str());
}
