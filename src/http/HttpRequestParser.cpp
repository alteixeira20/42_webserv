#include "http/HttpRequestParser.hpp"

#include <limits>

HttpRequestParser::HttpRequestParser()
	: _buffer(),
	  _request(),
	  _state(PARSING_REQUEST_LINE),
	  _errorStatus(0),
	  _headerBytes(0),
	  _maxHeaderSize(8192),
	  _contentLength(0),
	  _isChunked(false),
	  _chunkRemainder(0),
	  _chunkExpectingCrlf(false),
	  _decodedBody()
{
}

HttpRequestParser::HttpRequestParser(const HttpRequestParser &other)
	: _buffer(other._buffer),
	  _request(other._request),
	  _state(other._state),
	  _errorStatus(other._errorStatus),
	  _headerBytes(other._headerBytes),
	  _maxHeaderSize(other._maxHeaderSize),
	  _contentLength(other._contentLength),
	  _isChunked(other._isChunked),
	  _chunkRemainder(other._chunkRemainder),
	  _chunkExpectingCrlf(other._chunkExpectingCrlf),
	  _decodedBody(other._decodedBody)
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
		_contentLength = other._contentLength;
		_isChunked = other._isChunked;
		_chunkRemainder = other._chunkRemainder;
		_chunkExpectingCrlf = other._chunkExpectingCrlf;
		_decodedBody = other._decodedBody;
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
	_contentLength = 0;
	_isChunked = false;
	_chunkRemainder = 0;
	_chunkExpectingCrlf = false;
	_decodedBody.clear();
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

void	HttpRequestParser::parseAvailable()
{
	std::string	line;

	while (_state != PARSING_DONE && _state != PARSING_ERROR)
	{
		if (_state == PARSING_BODY)
			return (parseBodyAvailable());
		if (_state == PARSING_CHUNKED_BODY)
			return (parseChunkedBodyAvailable());
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
				finishHeaders();
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

	if (isContentLengthHeader(name)
		&& _request.hasHeader("content-length")
		&& _request.getHeader("content-length") != value)
		return (fail(400));

	_request.setHeader(name, value);
}

void	HttpRequestParser::finishHeaders()
{
	_isChunked = isChunkedTransferEncoding();

	if (_isChunked && _request.hasHeader("content-length"))
		return (fail(400));

	if (_isChunked)
	{
		_state = PARSING_CHUNKED_BODY;
		parseChunkedBodyAvailable();
		return ;
	}

	if (!parseContentLength())
		return ;
	if (_contentLength == 0)
		_state = PARSING_DONE;
	else
	{
		_state = PARSING_BODY;
		parseBodyAvailable();
	}
}

void	HttpRequestParser::parseBodyAvailable()
{
	if (_buffer.length() < _contentLength)
		return ;
	_request.setBody(_buffer.substr(0, _contentLength));
	_buffer.erase(0, _contentLength);
	_state = PARSING_DONE;
}

void	HttpRequestParser::parseChunkedBodyAvailable()
{
	std::string	line;
	std::size_t	size;

	while (_state == PARSING_CHUNKED_BODY)
	{
		if (_chunkExpectingCrlf)
		{
			if (_buffer.length() < 2)
				return ;
			if (_buffer[0] != '\r' || _buffer[1] != '\n')
				return (fail(400));
			_buffer.erase(0, 2);
			_chunkExpectingCrlf = false;
		}
		else if (_chunkRemainder > 0)
		{
			if (_buffer.length() < _chunkRemainder)
				return ;
			_decodedBody += _buffer.substr(0, _chunkRemainder);
			_buffer.erase(0, _chunkRemainder);
			_chunkRemainder = 0;
			_chunkExpectingCrlf = true;
		}
		else
		{
			if (!extractLine(line))
				return ;
			if (!parseChunkSize(line, size))
				return (fail(400));
			if (size == 0)
			{
				/* Last chunk: consume and discard trailer headers until empty line. */
				while (true)
				{
					if (!extractLine(line))
						return ;
					if (line.empty())
						break ;
				}
				_request.setBody(_decodedBody);
				_state = PARSING_DONE;
				return ;
			}
			_chunkRemainder = size;
		}
	}
}

bool	HttpRequestParser::parseContentLength()
{
	std::string				value;
	std::string::size_type	index;

	_contentLength = 0;
	if (!_request.hasHeader("content-length"))
		return (true);
	value = _request.getHeader("content-length");
	if (!isContentLengthValueValid(value))
	{
		fail(400);
		return (false);
	}
	index = 0;
	while (index < value.length())
	{
		_contentLength = _contentLength * 10 + value[index] - '0';
		++index;
	}

	return (true);
}

bool	HttpRequestParser::isContentLengthValueValid(
	const std::string &value) const
{
	std::size_t	index;
	std::size_t	length;

	if (value.empty())
		return (false);
	index = 0;
	length = 0;
	while (index < value.length())
	{
		if (value[index] < '0' || value[index] > '9')
			return (false);
		if (length > (std::numeric_limits<std::size_t>::max()
			- (value[index] - '0')) / 10)
			return (false);
		length = length * 10 + value[index] - '0';
		++index;
	}

	return (true);
}

bool	HttpRequestParser::isValidTarget(const std::string &target) const
{
	if (target.empty())
		return (false);

	if (target[0] != '/')
		return (false);

	return (true);
}

bool	HttpRequestParser::isContentLengthHeader(
	const std::string &name) const
{
	std::string				contentLength;
	std::string::size_type	index;
	char					current;

	contentLength = "content-length";
	if (name.length() != contentLength.length())
		return (false);
	index = 0;
	while (index < name.length())
	{
		current = name[index];
		if (current >= 'A' && current <= 'Z')
			current = current - 'A' + 'a';
		if (current != contentLength[index])
			return (false);
		++index;
	}

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

bool	HttpRequestParser::isChunkedTransferEncoding() const
{
	std::string				value;
	std::string				lower;
	std::string::size_type	index;

	if (!_request.hasHeader("transfer-encoding"))
		return (false);

	value = _request.getHeader("transfer-encoding");
	lower = trim(value);
	index = 0;
	while (index < lower.length())
	{
		if (lower[index] >= 'A' && lower[index] <= 'Z')
			lower[index] = lower[index] - 'A' + 'a';
		++index;
	}

	return (lower == "chunked");
}

bool	HttpRequestParser::parseChunkSize(
	const std::string &line, std::size_t &size) const
{
	std::string				hexPart;
	std::string::size_type	extPos;
	std::string::size_type	index;
	char					c;
	std::size_t				digit;

	extPos = line.find(';');
	hexPart = (extPos != std::string::npos) ? line.substr(0, extPos) : line;
	hexPart = trim(hexPart);

	if (hexPart.empty())
		return (false);

	size = 0;
	index = 0;
	while (index < hexPart.length())
	{
		c = hexPart[index];
		if (c >= '0' && c <= '9')
			digit = static_cast<std::size_t>(c - '0');
		else if (c >= 'a' && c <= 'f')
			digit = static_cast<std::size_t>(c - 'a' + 10);
		else if (c >= 'A' && c <= 'F')
			digit = static_cast<std::size_t>(c - 'A' + 10);
		else
			return (false);
		if (size > (std::numeric_limits<std::size_t>::max() - digit) / 16)
			return (false);
		size = size * 16 + digit;
		++index;
	}

	return (true);
}

void	HttpRequestParser::fail(unsigned int status)
{
	_state = PARSING_ERROR;
	_errorStatus = status;
}
