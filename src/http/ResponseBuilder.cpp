#include "http/ResponseBuilder.hpp"

#include <sstream>

ResponseBuilder::ResponseBuilder(void)
{
}

HttpResponse	ResponseBuilder::buildSimpleResponse(
	const HttpRequest &request) const
{
	HttpResponse	response;

	response.setStatus(200);
	response.setHeader("Content-Type", "text/plain");
	response.setCloseConnection(true);
	response.setBody(defaultBody(200, request.getTarget()));
	return (response);
}

HttpResponse	ResponseBuilder::buildErrorResponse(
	unsigned int statusCode) const
{
	HttpResponse	response;

	response.setStatus(statusCode);
	response.setHeader("Content-Type", "text/plain");
	response.setCloseConnection(true);
	response.setBody(defaultBody(statusCode, ""));
	return (response);
}

std::string	ResponseBuilder::defaultBody(unsigned int statusCode,
	const std::string &target)
{
	std::ostringstream	body;

	body << "webserv response\n";
	body << "status: " << statusCode << "\n";
	if (!target.empty())
		body << "target: " << target << "\n";
	return (body.str());
}
