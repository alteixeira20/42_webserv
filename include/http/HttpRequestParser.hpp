#ifndef HTTP_REQUEST_PARSER_HPP
# define HTTP_REQUEST_PARSER_HPP

# include <cstddef>
# include <string>
# include "http/HttpRequest.hpp"

/*
** Incremental HTTP/1.x request parser.
**
** The runtime feeds bytes into this parser. The parser owns protocol state,
** but never reads sockets, writes responses, resolves routes, or touches files.
*/

class HttpRequestParser
{
	public:
		enum ParserState
		{
			PARSING_REQUEST_LINE,
			PARSING_HEADERS,
			PARSING_DONE,
			PARSING_ERROR
		};

		HttpRequestParser();
		HttpRequestParser(const HttpRequestParser &other);
		HttpRequestParser	&operator=(const HttpRequestParser &other);
		~HttpRequestParser();

		void				reset();
		void				append(const std::string &data);

		bool				isComplete() const;
		bool				hasError() const;
		unsigned int		getErrorStatus() const;
		ParserState			getState() const;
		const HttpRequest	&getRequest() const;

	private:
		std::string			_buffer;
		HttpRequest			_request;
		ParserState			_state;
		unsigned int		_errorStatus;
		std::size_t			_headerBytes;
		std::size_t			_maxHeaderSize;

		void				parseAvailable();
		void				parseRequestLine(const std::string &line);
		void				parseHeaderLine(const std::string &line);
		void				fail(unsigned int status);

		bool				extractLine(std::string &line);
		bool				isValidTarget(const std::string &target) const;
		bool				isHeaderSectionTooLarge() const;
		std::string			trim(const std::string &value) const;
};

#endif
