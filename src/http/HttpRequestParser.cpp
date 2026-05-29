#include "http/HttpRequestParser.hpp"

HttpRequestParser::HttpRequestParser(void)
	: _buffer(),
	  _request(),
	  _state(PARSING_REQUEST_LINE),
	  _errorStatus(0)
{
}

HttpRequestParser::HttpRequestParser(const HttpRequestParser &other)
	: _buffer(other._buffer),
	  _request(other._request),
	  _state(other._state),
	  _errorStatus(other._errorStatus)
{
}

HttpRequestParser	&HttpRequestParser::operator=(
	const HttpRequestParser &other)
{
	if (this != &other)
	{
		_buffer = other._buffer;
		_request = other._request;
		_state = other._state;
		_errorStatus = other._errorStatus;
	}

	return (*this);
}

HttpRequestParser::~HttpRequestParser()
{
}

void	HttpRequestParser::reset()
{
	_buffer.clear();
	_request.clear();
	_state = PARSING_REQUEST_LINE;
	_errorStatus = 0;
}

void	HttpRequestParser::append(const std::string &data)
{
	if (_state == PARSING_DONE || _state == PARSING_ERROR)
		return ;

	_buffer += data;

	parseAvailable();
}

bool	HttpRequestParser::isComplete() const
{
	return (_state == PARSING_DONE);
}

bool	HttpRequestParser::hasError() const
{
	return (_state == PARSING_ERROR);
}

unsigned int	HttpRequestParser::getErrorStatus() const
{
	return (_errorStatus);
}

HttpRequestParser::ParserState	HttpRequestParser::getState() const
{
	return (_state);
}

const HttpRequest	&HttpRequestParser::getRequest() const
{
	return (_request);
}

/*
** This first parser slice only consumes the request line.
** Header parsing will extend this state machine in the next slice.
*/

void	HttpRequestParser::parseAvailable()
{
	std::string	line;

	if (_state != PARSING_REQUEST_LINE)
		return ;

	if (!extractLine(line))
		return ;

	parseRequestLine(line);

	if (_state != PARSING_ERROR)
		_state = PARSING_HEADERS;
}

bool	HttpRequestParser::extractLine(std::string &line)
{
	std::string::size_type	pos;

	pos = _buffer.find("\r\n");
	if (pos == std::string::npos)
		return (false);

	line = _buffer.substr(0, pos);
	_buffer.erase(0, pos + 2);

	return (true);
}

void	HttpRequestParser::parseRequestLine(const std::string &line)
{
	std::string::size_type	firstSpace;
	std::string::size_type	secondSpace;
	std::string				methodText;
	std::string				target;
	std::string				version;
	HttpMethod				method;

	firstSpace = line.find(' ');
	if (firstSpace == std::string::npos)
		return (fail(400));

	secondSpace = line.find(' ', firstSpace + 1);
	if (secondSpace == std::string::npos)
		return (fail(400));

	if (line.find(' ', secondSpace + 1) != std::string::npos)
		return (fail(400));

	methodText = line.substr(0, firstSpace);
	target = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
	version = line.substr(secondSpace + 1);
	method = parseHttpMethod(methodText);

	if (!isSupportedHttpMethod(method))
		return (fail(405));
	if (!isValidTarget(target))
		return (fail(400));
	if (version != "HTTP/1.1")
		return (fail(505));

	_request.setMethod(method);
	_request.setTarget(target);
	_request.setVersion(version);
}

bool	HttpRequestParser::isValidTarget(const std::string &target) const
{
	if (target.empty())
		return (false);
	if (target[0] != '/')
		return (false);

	return (true);
}

void	HttpRequestParser::fail(unsigned int status)
{
	_state = PARSING_ERROR;
	_errorStatus = status;
}
