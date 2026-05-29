#include "http/HttpRequestParser.hpp"

HttpRequestParser::HttpRequestParser()
	: _buffer(),
	  _request(),
	  _state(PARSING_REQUEST_LINE),
	  _errorStatus(0),
	  _headerBytes(0),
	  _maxHeaderSize(8192)
{
}

HttpRequestParser::HttpRequestParser(const HttpRequestParser &other)
	: _buffer(other._buffer),
	  _request(other._request),
	  _state(other._state),
	  _errorStatus(other._errorStatus),
	  _headerBytes(other._headerBytes),
	  _maxHeaderSize(other._maxHeaderSize)
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
		_headerBytes = other._headerBytes;
		_maxHeaderSize = other._maxHeaderSize;
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
	_headerBytes = 0;
}

void	HttpRequestParser::append(const std::string &data)
{
	if (_state == PARSING_DONE || _state == PARSING_ERROR)
		return ;
	_buffer += data;
	if (isHeaderSectionTooLarge())
		return (fail(431));
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
** Consume as many complete protocol lines as are currently available.
** Body parsing will extend this state machine later.
*/
void	HttpRequestParser::parseAvailable()
{
	std::string	line;

	while (_state != PARSING_DONE && _state != PARSING_ERROR)
	{
		if (!extractLine(line))
			return ;
		if (_state == PARSING_REQUEST_LINE)
		{
			parseRequestLine(line);
			if (_state != PARSING_ERROR)
				_state = PARSING_HEADERS;
		}
		else if (_state == PARSING_HEADERS)
		{
			_headerBytes += line.length() + 2;
			if (_headerBytes > _maxHeaderSize)
				return (fail(431));
			if (line.empty())
				_state = PARSING_DONE;
			else
				parseHeaderLine(line);
		}
	}
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

void	HttpRequestParser::parseHeaderLine(const std::string &line)
{
	std::string::size_type	colon;
	std::string				name;
	std::string				value;

	colon = line.find(':');
	if (colon == std::string::npos)
		return (fail(400));

	name = trim(line.substr(0, colon));
	value = trim(line.substr(colon + 1));

	if (name.empty())
		return (fail(400));

	_request.setHeader(name, value);
}

bool	HttpRequestParser::isValidTarget(const std::string &target) const
{
	if (target.empty())
		return (false);

	if (target[0] != '/')
		return (false);

	return (true);
}

bool	HttpRequestParser::isHeaderSectionTooLarge() const
{
	if (_state != PARSING_HEADERS)
		return (false);

	return (_headerBytes + _buffer.length() > _maxHeaderSize);
}

std::string	HttpRequestParser::trim(const std::string &value) const
{
	std::string::size_type	start;
	std::string::size_type	end;

	start = 0;
	while (start < value.length()
		&& (value[start] == ' ' || value[start] == '\t'))
		++start;

	end = value.length();
	while (end > start
		&& (value[end - 1] == ' ' || value[end - 1] == '\t'))
		--end;

	return (value.substr(start, end - start));
}

void	HttpRequestParser::fail(unsigned int status)
{
	_state = PARSING_ERROR;
	_errorStatus = status;
}
