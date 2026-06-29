#ifndef HTTP_REQUEST_EVALUATOR_HPP
# define HTTP_REQUEST_EVALUATOR_HPP

#include <string>
#include "http/HttpRequest.hpp"
#include "http/HttpRequestResolver.hpp"

/*
** HttpRequestEvaluator takes a parsed request and a resolved route and
** decides what should happen next.
**
** It does not generate responses, read files, or touch the filesystem.
** It does not modify config or runtime state.
*/

class HttpRequestEvaluator
{
	public:
		enum Action
		{
			CONTINUE,
			REDIRECT,
			ERROR
		};

		struct Decision
		{
			Action			action;
			unsigned int	statusCode;
			std::string		redirectLocation;
		};

		HttpRequestEvaluator();
		HttpRequestEvaluator(const HttpRequestEvaluator &other);
		HttpRequestEvaluator	&operator=(const HttpRequestEvaluator &other);
		~HttpRequestEvaluator();

		Decision	evaluate(const HttpRequest &request,
						const HttpRequestResolver::Result &resolved) const;
};

#endif
