#ifndef RESPONSE_BUILDER_HPP
# define RESPONSE_BUILDER_HPP

# include "http/HttpRequest.hpp"
# include "http/HttpResponse.hpp"

class ResponseBuilder
{
public:
	ResponseBuilder(void);

	HttpResponse	buildSimpleResponse(const HttpRequest &request) const;
	HttpResponse	buildErrorResponse(unsigned int statusCode) const;
	HttpResponse	buildCustomErrorResponse(unsigned int statusCode,
						const std::string &body,
						const std::string &contentType) const;

private:
	static std::string	defaultBody(unsigned int statusCode,
							const std::string &target);
	static std::string	defaultErrorBody(unsigned int statusCode,
							const std::string &reasonPhrase);
};

#endif
