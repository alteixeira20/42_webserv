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
	response.setHeader("Content-Type", "text/html");
	response.setCloseConnection(true);
	response.setBody(defaultErrorBody(response.statusCode(),
			response.reasonPhrase()));
	return (response);
}

HttpResponse	ResponseBuilder::buildCustomErrorResponse(
	unsigned int statusCode, const std::string &body,
	const std::string &contentType) const
{
	HttpResponse	response;

	response.setStatus(statusCode);
	response.setHeader("Content-Type", contentType);
	response.setCloseConnection(true);
	response.setBody(body);
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

std::string	ResponseBuilder::defaultErrorBody(unsigned int statusCode,
	const std::string &reasonPhrase)
{
	std::ostringstream	body;

	body << "<!doctype html>\n";
	body << "<html>\n";
	body << "<head><title>" << statusCode << " " << reasonPhrase
		<< "</title></head>\n";
	body << "<body>\n";
	body << "<h1>" << statusCode << " " << reasonPhrase << "</h1>\n";
	body << "</body>\n";
	body << "</html>\n";
	return (body.str());
}
