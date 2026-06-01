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

private:
	static std::string	defaultBody(unsigned int statusCode,
							const std::string &target);
};

#endif
